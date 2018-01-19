// -*- coding: utf-8 -*-
//
// fgrcc.cxx --- Simple resource compiler for FlightGear
// Copyright (C) 2017  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <ios>                  // std::basic_ios, std::streamsize...
#include <string>
#include <array>
#include <tuple>
#include <vector>
#include <iostream>             // std::ios_base, std::cerr, etc.
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <numeric>              // std::accumulate()
#include <functional>
#include <type_traits>          // std::underlying_type
#include <stdexcept>
#include <cstdlib>
#include <cstddef>              // std::size_t
#include <clocale>
#include <cstring>
#include <cerrno>
#include <cassert>

#include <zlib.h>               // Z_BEST_COMPRESSION

#include <simgear/embedded_resources/EmbeddedResource.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/argparse.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/xml/easyxml.hxx>

#include "fgrcc.hxx"

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using simgear::enumValue;

// The name is still hard-coded essentially in the text for --help
// (cf. showUsage()), because the formatting there depends on how long the
// name is, and it makes said text more readable and maintainable.
static const string PROGNAME = "fgrcc";

// 'stuff' should insert UTF-8-encoded text into the stream
#define LOG(stuff) do { cerr << PROGNAME << ": " << stuff << "\n"; } while(0)

static string prettyPrintNbOfBytes(std::size_t nbBytes)
{
  std::ostringstream oss;

  oss << std::fixed << std::setprecision(1);
  if (nbBytes >= 1024*1024) {
    oss << static_cast<float>(nbBytes) / (1024*1024) << " MiB";
  } else if (nbBytes >= 1024) {
    oss << static_cast<float>(nbBytes) / 1024 << " KiB";
  } else {
    oss << nbBytes << " byte" << ((nbBytes == 1) ? "" : "s");
  }

  return oss.str();
}

static SGPath assembleVirtualPath(string firstPart, const string& secondPart)
{
  if (!simgear::strutils::ends_with(firstPart, "/")) {
    firstPart += '/';
  }

  assert( !(simgear::strutils::starts_with(secondPart, "/") ||
            simgear::strutils::ends_with(secondPart, "/")) );
  SGPath virtualPath = SGPath::fromUtf8(firstPart + secondPart);

  return virtualPath;
}

// ***************************************************************************
// *                           ResourceDeclaration                           *
// ***************************************************************************
ResourceDeclaration::ResourceDeclaration(
  const SGPath& virtualPath_, const SGPath& realPath_,
  const std::string& language_,
  simgear::AbstractEmbeddedResource::CompressionType compressionType_)
  : virtualPath(virtualPath_),
    realPath(realPath_),
    language(language_),
    compressionType(compressionType_)
{ }

bool ResourceDeclaration::isCompressed() const
{
  bool res;

  switch (compressionType) {
  case simgear::AbstractEmbeddedResource::CompressionType::ZLIB:
    res = true;
    break;
  case simgear::AbstractEmbeddedResource::CompressionType::NONE:
    res = false;
    break;
  default:
    throw sg_exception("bug: unexpected compression type for an embedded "
                       "resource: " +
                       std::to_string(enumValue(compressionType)));
  }

  return res;
}

// ************************************************************************
// *                      ResourceBuilderXMLVisitor                       *
// ************************************************************************

// Initialization of static members
const std::array<string, 2> ResourceBuilderXMLVisitor::_tagTypeStr = {
  {"start",
   "end"}
};
const std::array<string, 5> ResourceBuilderXMLVisitor::_parserStateStr = {
  {"before 'FGRCC' element",
   "inside 'FGRCC' element",
   "inside 'qresource' element",
   "inside 'file' element",
   "after 'FGRCC' element"
  }
};

ResourceBuilderXMLVisitor::ResourceBuilderXMLVisitor(const SGPath& rootDir)
  : _rootDir(rootDir)
{ }

const vector<ResourceDeclaration>&
ResourceBuilderXMLVisitor::getResourceDeclarations() const
{
  return _resourceDeclarations;
}

// Static method
bool
ResourceBuilderXMLVisitor::readBoolean(const string& s)
{
  if (s == "yes" || s == "true" || s == "1") {
    return true;
  } else if (s == "no" || s == "false" || s == "0") {
    return false;
  } else {
    throw sg_exception(
      "invalid value for a boolean attribute: '" + s + "'. Authorized values "
      "are 'yes', 'no', 'true', 'false', '1' and '0'.");
  }
}

// Static method
simgear::AbstractEmbeddedResource::CompressionType
ResourceBuilderXMLVisitor::determineCompressionType(
  const SGPath& resFilePath, const std::string& compression)
{
  const std::unordered_set<std::string> extsWithNoCompression = {
    "png", "jpg", "jpeg", "gz", "bz2", "xz", "lzma", "zip" };
  const std::string ext = resFilePath.lower_extension();
  simgear::AbstractEmbeddedResource::CompressionType res;

  if (compression == "none") {
    res = simgear::AbstractEmbeddedResource::CompressionType::NONE;
  } else if (compression == "zlib") {
    res = simgear::AbstractEmbeddedResource::CompressionType::ZLIB;
  } else if (compression == "auto") {
    res = (extsWithNoCompression.count(ext) > 0) ?
      simgear::AbstractEmbeddedResource::CompressionType::NONE :
      simgear::AbstractEmbeddedResource::CompressionType::ZLIB;
  } else {
    throw sg_exception(
      "invalid value for the 'compression' attribute: '" + compression + "'");
  }

  return res;
}

[[ noreturn ]] void
ResourceBuilderXMLVisitor::unexpectedTagError(
  XMLTagType tagType, const string& found, const string& expected)
{
  std::ostringstream oss;
  string final = expected.empty() ? "" :
    " (expected '" + expected + "' instead)";

  savePosition();
  oss << getPath() << ":" << getLine() <<  ":" << getColumn() <<
    ": unexpected " << _tagTypeStr[enumValue(tagType)] << " tag: '" << found <<
    "'" << final;

  throw sg_exception(oss.str());
}

void
ResourceBuilderXMLVisitor::startElement(const char* name,
                                        const XMLAttributes& atts)
{
  switch (_parserState) {
  case ParserState::START:
    if (!std::strcmp(name, "FGRCC")) {
      _parserState = ParserState::INSIDE_FGRCC_ELT;
    } else {
      unexpectedTagError(XMLTagType::START, name, "FGRCC");
    }
    break;
  case ParserState::INSIDE_FGRCC_ELT:
    if (!std::strcmp(name, "qresource")) {
      startQResourceElement(atts);
      _parserState = ParserState::INSIDE_QRESOURCE_ELT;
    } else {
      unexpectedTagError(XMLTagType::START, name, "qresource");
    }
    break;
  case ParserState::INSIDE_QRESOURCE_ELT:
    if (!std::strcmp(name, "file")) {
      startFileElement(atts);
      _parserState = ParserState::INSIDE_FILE_ELT;
    } else {
      unexpectedTagError(XMLTagType::START, name, "file");
    }
    break;
  case ParserState::INSIDE_FILE_ELT:
    unexpectedTagError(XMLTagType::START, name); // throws an exception
  case ParserState::END:
    unexpectedTagError(XMLTagType::START, name); // throws an exception
  default:
    throw std::logic_error(
      "unexpected state reached in resource file parser: " +
      std::to_string(enumValue(_parserState)));
  }
}

void
ResourceBuilderXMLVisitor::endElement(const char *name)
{
  switch (_parserState) {
  case ParserState::START:
    unexpectedTagError(XMLTagType::END, name); // throws an exception
  case ParserState::INSIDE_FGRCC_ELT:
    if (!std::strcmp(name, "FGRCC")) {
      _parserState = ParserState::END;
    } else {
      unexpectedTagError(XMLTagType::END, name, "FGRCC");
    }
    break;
  case ParserState::INSIDE_QRESOURCE_ELT:
    if (!std::strcmp(name, "qresource")) {
      _parserState = ParserState::INSIDE_FGRCC_ELT;
    } else {
      unexpectedTagError(XMLTagType::END, name, "qresource");
    }
    break;
  case ParserState::INSIDE_FILE_ELT:
    if (!std::strcmp(name, "file")) {
      // First do some sanity checks, then assemble the resource virtual path
      const auto throwError = [this]
        (const string& contents, const string& message)
      {
        savePosition();
        std::ostringstream oss;
        oss << this->getPath() << ":" << this->getLine() <<  ":" <<
          this->getColumn() << ": invalid contents for a <file> element " <<
          message;
        throw sg_format_exception(oss.str(), contents);
      };

      if (_resourceFile.empty()) {
        throwError(_resourceFile, "(empty)");
      } else if (simgear::strutils::ends_with(_resourceFile, "/")) {
        throwError(_resourceFile,
                   "(ending with a '/'): '" + _resourceFile + "'");
      }

      const string secondPart = (_currentAlias.empty() ? _resourceFile :
                                 _currentAlias);
      // Make sure we don't get double slashes and similar problems
      SGPath virtualPath = assembleVirtualPath(_currentPrefix, secondPart);

      // We have to be careful here, because SGPath::append() and
      // SGPath::operator/() don't behave in the expected way when the path is
      // just '/' (they always insert a '/' before the second part, if it
      // doesn't itself start with a '/').
      SGPath p = _rootDir;
      if (p.utf8Str() == "/") {
        p = SGPath();
      }
      const SGPath realPath = p / _resourceFile;

      const auto compressionType = determineCompressionType(
        realPath, _currentCompressionTypeStr);
      // Record what we've gathered for later processing (this way, we'll
      // only start writing to the output stream if the input is entirely
      // correct).
      _resourceDeclarations.emplace_back(
        virtualPath, realPath, _currentLanguage, compressionType);

      _parserState = ParserState::INSIDE_QRESOURCE_ELT;
    } else {
      unexpectedTagError(XMLTagType::END, name, "file");
    }
    break;
  case ParserState::END:
    unexpectedTagError(XMLTagType::END, name); // throws an exception
  default:
    throw std::logic_error(
      "unexpected state reached in resource file parser: " +
      std::to_string(enumValue(_parserState)));
  }
}

void
ResourceBuilderXMLVisitor::startQResourceElement(const XMLAttributes &atts)
{
  const char *prefix = atts.getValue("prefix");
  // Make a copy, as 'atts' is short-lived.
  _currentPrefix = string(prefix ? prefix : "/");

  // Make sure all virtual path prefixes are normalized
  if (!simgear::strutils::starts_with(_currentPrefix, "/") ||
      (_currentPrefix != "/" && simgear::strutils::ends_with(_currentPrefix,
                                                             "/"))) {
    savePosition();
    std::ostringstream oss;
    oss << getPath() << ":" << getLine() << ": invalid 'prefix' attribute: '" <<
      _currentPrefix << "' (must start with a '/' and not end with a '/', "
      "unless equal to the one-char prefix '/')";
    throw sg_format_exception(oss.str(), _currentPrefix);
  }

  const char *lang = atts.getValue("lang");
  _currentLanguage = string(lang ? lang : "");
}

void
ResourceBuilderXMLVisitor::startFileElement(const XMLAttributes &atts)
{
  const char *alias = atts.getValue("alias");

  if (alias && !std::strcmp(alias, "")) {
    std::ostringstream oss;
    oss << getPath() << ":" << getLine() << ": invalid empty 'alias' attribute";
    throw sg_format_exception(oss.str(), string(alias));
  }

  // cf. comment in startQResourceElement()
  _currentAlias = string(alias ? alias : "");

  const auto checkForError = [this]
    (const string& valueToTest, const string& startingOrEnding,
     std::function<bool(const string&, const string &)> testFunc)
    {
      if (testFunc(valueToTest, "/")) {
        this->savePosition();
        std::ostringstream oss;
        oss << this->getPath() << ":" << this->getLine() <<
        ": invalid 'alias' attribute " << startingOrEnding <<
        " with a '/': '" << valueToTest << "'";
        throw sg_format_exception(oss.str(), valueToTest);
      }
    };

  checkForError(_currentAlias, "starting", simgear::strutils::starts_with);
  checkForError(_currentAlias, "ending", simgear::strutils::ends_with);

  // This attribute is not part of the v1.0 QRC format, it's a FlightGear
  // extension.
  const char *compress = atts.getValue("compression");
  _currentCompressionTypeStr = string(compress ? compress : "auto");

  // Start assembling a new file path (it may come in several chunks)
  _resourceFile.clear();
}

void
ResourceBuilderXMLVisitor::data(const char *s, int len)
{
  string chunk(s, len);

  if (_parserState == ParserState::INSIDE_FILE_ELT) {
    if (_resourceFile.empty() && simgear::strutils::starts_with(chunk, "/")) {
      savePosition();
      std::ostringstream oss;
      oss << getPath() << ":" << getLine() <<  ":" << getColumn() <<
        ": invalid <file> element (contents starting with a '/'): " << chunk <<
        "...";
      throw sg_format_exception(oss.str(), chunk);
    }

    _resourceFile += chunk;
  } else if (chunk.find_first_not_of(" \t\n") != string::npos) {
    // We are not inside a <file> element and we found character data that
    // contains something different from spaces, tabs and newlines -> this is
    // invalid.
    std::ostringstream oss;

    savePosition();
    oss << getPath() << ":" << getLine() <<  ":" << getColumn() <<
      ": unexpected character data " <<
      _parserStateStr[enumValue(_parserState)] << ": '" << string(s, len) <<
      "'";

    throw sg_exception(oss.str());
  }
}

void
ResourceBuilderXMLVisitor::warning(const char *message, int line, int column)
{
  LOG("warning: " << getPath() << ": " << line << ":" << column << ": " <<
      message);
}

void
ResourceBuilderXMLVisitor::error(const char *message, int line, int column)
{
  std::ostringstream oss;
  oss << getPath() << ": " << line << ":" << column << ": " << message;

  throw sg_exception(oss.str());
}

// ************************************************************************
// *                      Writing the generated code                      *
// ************************************************************************

CPPEncoder::CPPEncoder(std::istream& inputStream)
  : _inputStream(inputStream)
{ }

// Static method
[[ noreturn ]] void CPPEncoder::handleWriteError(int errorNumber)
{
  throw sg_exception(
    "error while writing octal-encoded resource data: " +
    simgear::strutils::error_string(errorNumber));
}

// Extract bytes from a stream, write them in lines of octal-encoded C++
// character escapes. Return the number of bytes from the input stream that
// have been encoded.
std::size_t CPPEncoder::write(std::ostream& oStream)
{
  char buf[4096];
  std::streamsize nbBytesRead;
  // Lines will be at most 80 characters wide because of the '";' following
  // the string literal contents.
  const std::size_t maxColumns = 78;
  std::size_t availableColumns = 0; // what remains to fill for current line
  int savedErrno;
  std::size_t payloadSize = 0;

  do {
    // std::ifstream::read() sets *both* the eofbit and failbit flags if EOF
    // was reached before it could read the number of characters requested.
    _inputStream.read(buf, sizeof(buf));
    savedErrno = errno;
    nbBytesRead = _inputStream.gcount();
    auto charPtr = reinterpret_cast<const unsigned char*>(buf);

    // Process what has been read (*even* if _inputStream.fail() is true)
    for (std::streamsize remaining = nbBytesRead; remaining > 0; remaining--) {
      std::ostringstream oss;
      oss << "\\" << std::oct << static_cast<unsigned int>(*charPtr++);
      string theCharLiteral = oss.str();

      if (availableColumns < theCharLiteral.size()) {
        if (!(oStream << "\\\n")) {
          handleWriteError(errno);
        }

        availableColumns = maxColumns;
      }

      if (!(oStream << theCharLiteral)) {
        handleWriteError(errno);
      }
      availableColumns -= theCharLiteral.size();
    } // of for loop over the 'nbBytesRead' bytes

    payloadSize += nbBytesRead;
  } while (_inputStream);

  if (_inputStream.bad()) {
    throw sg_exception(
      "error while reading from the possibly-compressed resource stream: " +
      simgear::strutils::error_string(savedErrno));
  }

  return payloadSize;
}

// Weaker interface that might be convenient in some cases...
std::ostream& operator<<(std::ostream& outputStream, CPPEncoder& cppEncoder)
{
  cppEncoder.write(outputStream);
  return outputStream;
}

// ***************************************************************************
// *                       ResourceCodeGenerator class                       *
// ***************************************************************************

ResourceCodeGenerator::ResourceCodeGenerator(
  const vector<ResourceDeclaration>& resourceDeclarations,
  std::ostream& outputStream,
  const SGPath& outputCppFile,
  const string& initFuncName,
  const SGPath& outputHeaderFile,
  const string& headerIdentifier,
  std::size_t compInBufSize,
  std::size_t compOutBufSize)
  : _resDecl(resourceDeclarations),
    _outputStream(outputStream),
    _outputCppFile(outputCppFile),
    _initFuncName(initFuncName),
    _outputHeaderFile(outputHeaderFile),
    _headerIdentifier(headerIdentifier),
    _compInBufSize(compInBufSize),
    _compOutBufSize(compOutBufSize),
    _compressionInBuf(new char[_compInBufSize]),
    _compressionOutBuf(new char[_compOutBufSize])
{ }

std::size_t ResourceCodeGenerator::writeEncodedResourceContents(
  const ResourceDeclaration& resDecl) const
{
  std::unique_ptr<std::istream> iFileStream_p(
    static_cast<std::istream *>(new sg_ifstream(resDecl.realPath)));

  if (! *iFileStream_p) {
    throw sg_exception("unable to open file '" + resDecl.realPath.utf8Str() +
                       "': " + simgear::strutils::error_string(errno));
  }

  std::unique_ptr<std::istream> iStream_p;

  switch (resDecl.compressionType) {
  case simgear::AbstractEmbeddedResource::CompressionType::ZLIB:
    iStream_p.reset(
      static_cast<std::istream *>(
        new simgear::ZlibCompressorIStream(
          std::move(iFileStream_p), resDecl.realPath, Z_BEST_COMPRESSION,
          simgear::ZLibCompressionFormat::ZLIB,
          simgear::ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
          &_compressionInBuf[0], _compInBufSize,
          &_compressionOutBuf[0], _compOutBufSize, /* putbackSize */ 0)));
    break;
  case simgear::AbstractEmbeddedResource::CompressionType::NONE:
    iStream_p = std::move(iFileStream_p);
    break;
  default:
    throw sg_exception("bug: unexpected compression type for an embedded "
                       "resource: " +
                       std::to_string(enumValue(resDecl.compressionType)));
  }

  // Throws in case of an error
  return CPPEncoder(*iStream_p).write(_outputStream);
}

// Static method
string ResourceCodeGenerator::resourceClass(
  simgear::AbstractEmbeddedResource::CompressionType compressionType)
{
  string resClass;

  switch (compressionType) {
  case simgear::AbstractEmbeddedResource::CompressionType::ZLIB:
    resClass = "ZlibEmbeddedResource";
    break;
  case simgear::AbstractEmbeddedResource::CompressionType::NONE:
    resClass = "RawEmbeddedResource";
    break;
  default:
    throw sg_exception("bug: unexpected compression type for an embedded "
                       "resource: "
                       + std::to_string(enumValue(compressionType)));
  }

  return resClass;
}

// Static method
//
// Encode an integral index in a way that can be safely used as part of a C++
// variable name. This is base 26 written from right to left (most significant
// digit last).
string ResourceCodeGenerator::encodeResourceIndex(std::size_t index) {
  string res;
  std::size_t remainder;

  if (index == 0) {
    res = string("A");          // 0
  }

  while (index > 0) {
    remainder = index % 26;
    res += 'A' + remainder;     // append a digit
    index /= 26;
  }

  return res;
}

void ResourceCodeGenerator::writeCode() const
{
  // This exception is not usable on all systems (cf.
  // <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66145>), but this bug
  // will eventually go away, and in the meantime, it's acceptable here that
  // the exception can't be caught on buggy systems. Other alternatives are
  // all worse IMHO.
  _outputStream.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  string msg = (_outputCppFile.isNull()) ?
    "writing C++ contents to the standard output" :
    "writing C++ file: '" + _outputCppFile.utf8Str() + "'";
  LOG(msg);

  _outputStream << "\
// -*- coding: utf-8 -*-\n\
//\n\
// File automatically generated by " << PROGNAME << ".\n  \
\n\
#include <memory>\n\
#include <utility>\n\
\n\
#include <simgear/io/iostreams/CharArrayStream.hxx>\n\
#include <simgear/io/iostreams/zlibstream.hxx>\n\
#include <simgear/embedded_resources/EmbeddedResource.hxx>\n\
#include <simgear/embedded_resources/EmbeddedResourceManager.hxx>\n\
\n\
using std::unique_ptr;\n\
using simgear::AbstractEmbeddedResource;\n\
using simgear::RawEmbeddedResource;\n\
using simgear::ZlibEmbeddedResource;\n\
using simgear::EmbeddedResourceManager;\n";

  // If the resource is compressed, this is the compressed size.
  vector<std::size_t> resSizeInBytes;

  for (vector<ResourceDeclaration>::size_type resNum = 0;
       resNum < _resDecl.size(); resNum++) {
    const auto& resDcl = _resDecl[resNum];
    _outputStream << "\nstatic const char resource" <<
      encodeResourceIndex(resNum) << "[] = \"";
    resSizeInBytes.push_back(writeEncodedResourceContents(resDcl));
    _outputStream << "\";\n";
  }

  _outputStream << "\n"
    "void " << _initFuncName << "()\n"
    "{\n" <<
    ((_resDecl.empty()) ? "  " : "  const auto& resMgr = ") <<
    "EmbeddedResourceManager::instance();\n";

  for (vector<ResourceDeclaration>::size_type resNum = 0;
       resNum < _resDecl.size(); resNum++) {
    const auto& resDcl = _resDecl[resNum];
    string resClass = resourceClass(resDcl.compressionType);
    string encodedResNum = encodeResourceIndex(resNum);

    _outputStream.flags(std::ios::dec);
    _outputStream << "\n  unique_ptr<const " << resClass << "> res" <<
      encodedResNum << "(\n    new " << resClass << "(";
    std::ostringstream resConstructArgs;

    switch (resDcl.compressionType) {
    case simgear::AbstractEmbeddedResource::CompressionType::ZLIB:
      resConstructArgs << "resource" << encodedResNum << ", " <<
        resSizeInBytes[resNum] << ", " << resDcl.realPath.sizeInBytes();
      break;
    case simgear::AbstractEmbeddedResource::CompressionType::NONE:
      resConstructArgs << "resource" << encodedResNum << ", " <<
        resDcl.realPath.sizeInBytes();
      break;
    default:
      throw sg_exception(
        "bug: unexpected compression type for an embedded resource: " +
        std::to_string(enumValue(resDcl.compressionType)));
    }

    // Use UTF-8 as the output encoding
    _outputStream << resConstructArgs.str() <<
      "));\n  resMgr->addResource("
      "\"" << simgear::strutils::escape(resDcl.virtualPath.utf8Str()) << "\", "
      "std::move(" << "res" << encodedResNum << ")";

    if (!resDcl.language.empty()) {
      _outputStream << ", \"" << simgear::strutils::escape(resDcl.language) <<
        "\"";
    }

    _outputStream << ");\n";

    // Print a log message about the resource we just added
    std::ostringstream oss;
    oss << "added '" << resDcl.realPath.utf8Str() << "' (";

    if (resDcl.isCompressed()) {
      oss << prettyPrintNbOfBytes(resDcl.realPath.sizeInBytes()) <<
        "; compressed: " << prettyPrintNbOfBytes(resSizeInBytes[resNum]) <<
        ")";
    } else {
      oss << prettyPrintNbOfBytes(resSizeInBytes[resNum]) << ")";
    }

    LOG(oss.str());
  }

  _outputStream << "}\n";

  // Print the total size of resources
  std::size_t staticMemoryUsedByResources = std::accumulate(
    resSizeInBytes.begin(), resSizeInBytes.end(), std::size_t(0));
  LOG("static memory used by resources (total): " <<
      prettyPrintNbOfBytes(staticMemoryUsedByResources));

  if (!_outputHeaderFile.isNull()) {
    writeHeaderFile();
  }
}

void ResourceCodeGenerator::writeHeaderFile() const
{
  assert(!_outputHeaderFile.isNull());
  sg_ofstream outFile(_outputHeaderFile);

  if (!outFile) {
    throw sg_exception("unable to open output header file '" +
                       _outputHeaderFile.utf8Str() + "': " +
                       simgear::strutils::error_string(errno));
  }

  outFile.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  LOG("writing header file: '" << _outputHeaderFile.utf8Str() << "'");
  outFile << "\
// -*- coding: utf-8 -*-\n\
//\n\
// Header file automatically generated by " << PROGNAME << ".\n   \
\n\
#ifndef " << _headerIdentifier << "\n\
#define " << _headerIdentifier << "\n\
\n\
void " << _initFuncName << "();\n\
\n\
#endif  // of " << _headerIdentifier << "\n";
}

// ************************************************************************
// *                           Other functions                            *
// ************************************************************************

void showUsage(std::ostream& os) {
  os << "Usage: " << PROGNAME << " [OPTION...] INFILE\n"
    "\
Compile resources declared in INFILE, into C++ code.\n\
\n\
INFILE should be a file in XML format declaring a set of resources. Each\n\
resource has a contents that is initially read from a file, and a virtual\n\
path that will be used for retrieval of the resource contents via the\n\
EmbeddedResourceManager. The real path of a resource (that allows 'fgrcc' to\n\
retrieve the resource data), its virtual path as well as other attributes\n\
are all declared in INPUT.\n\
\n\
For each resource declared in INPUT, 'fgrcc' thus reads metadata (virtual\n\
path, language attribute...) and contents from the associated file. Then, it\n\
generates C++ code that can be used to register the resources with SimGear's\n\
EmbeddedResourceManager, of which FlightGear has an instance. In this\n\
generated C++ code, the contents of each resource is represented by a static\n\
array of const char. It is compressed by default, except for a few file\n\
extensions (png, jpg, jpeg, gz, bz2...).\n\
\n\
The EmbeddedResourceManager\n\
---------------------------\n\
\n\
The EmbeddedResourceManager is a SimGear class that provides several ways to\n\
access the contents of a given resource. The simplest way is to retrieve the\n\
resource contents as an std::string (this works for all kinds of resources,\n\
be they text or binary). This method may be undesirable for large resources\n\
though[1], because std::string contents is stored in dynamically allocated\n\
memory, and therefore the creation of an std::string instance to hold the\n\
resource contents always makes a copy of this contents (with automatic, on\n\
the fly decompression for compressed resources).\n\
\n\
  [1] Which may be undesirable per se anyway, since they have to be stored\n\
      in static memory.\n\
\n\
The EmbeddedResourceManager class offers a few methods that are more\n\
memory-friendly for large resources: by using SimGear classes such as\n\
CharArrayIStream and ZlibDecompressorIStream, it gives zero-copy,\n\
incremental access to resource contents, with transparent decompression in\n\
the case of compressed resources. These two classes are derived from\n\
std::istream, therefore this contents can be easily processed with standard\n\
C++ techniques. For highest performance (using lower-level methods), the\n\
EmbeddedResourceManager also provides access to resource data via an\n\
std::streambuf interface, by means of classes such as ROCharArrayStreambuf\n\
and ZlibDecompressorIStreambuf.\n\
\n\
Format of the resource declaration file\n\
---------------------------------------\n\
\n\
The supported format for INFILE is a thin superset of the v1.0 QRC format\n\
used by Qt (<http://doc.qt.io/qt-5/resources.html>). The differences with\n\
this QRC format are:\n\
\n\
  1. The <!DOCTYPE RCC> declaration at the beginning should be omitted (or\n\
     replaced with <!DOCTYPE FGRCC>, however such a DTD currently doesn't\n\
     exist). I suggest to add an XML declaration instead, for instance:\n\
\n\
       <?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
\n\
  2. <RCC> and </RCC> must be replaced with <FGRCC> and </FGRCC>,\n\
     respectively.\n\
\n\
  3. The FGRCC format supports a 'compression' attribute for each 'file'\n\
     element. At the time of this writing, the allowed values for this\n\
     attribute are 'none', 'zlib' and 'auto'. When set to a value that is\n\
     not 'auto', this attribute of course bypasses the algorithm for\n\
     determining whether and how to compress a given resource (algorithm\n\
     which relies on the file extension).\n\
\n\
  4. Resource paths (paths to the real files, not virtual paths) are\n\
     interpreted relatively to the directory specified with the --root\n\
     option. If this option is not passed to 'fgrcc', then the default root\n\
     directory is the one containing INFILE, which matches the behavior of\n\
     Qt's 'rcc' tool.\n\
\n\
Here follows a sample resource declaration file. In the comments, we use\n\
$ROOT to represent the folder specified with --root (this $ROOT notation is\n\
only a placeholder used to explain the concepts here, it is *not* syntax\n\
understood by 'fgrcc'!).\n\
\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
\n\
<FGRCC version=\"1.0\">\n\
  <qresource>\n\
    <!-- The contents of '$ROOT/path/to/a/file' will be served by the\n\
         EmbeddedResourceManager under the virtual path '/path/to/a/file'\n\
         (anchored to '/' because we didn't define any prefix; see below).\n\
    -->\n\
    <file>path/to/a/file</file>\n\
    <file compression=\"none\">another/file (won't be compressed)</file>\n\
    <!-- This one will have the virtual path '/foobar/intro.txt'. -->\n\
    <file alias=\"foobar/intro.txt\">yet another/file</file>\n\
  </qresource>\n\
\n\
  <qresource prefix=\"/some/prefix\">\n\
    <!-- The contents of '$ROOT/path/to/file1' will be served by the\n\
         EmbeddedResourceManager under the virtual path\n\
         '/some/prefix/path/to/file1'. -->\n\
    <file>path/to/file1</file>\n\
    <!-- The contents of '$ROOT/other/file' will be accessible through the\n\
         virtual path '/some/prefix/my/alias'. -->\n\
    <file alias=\"my/alias\">other/file</file>\n\
  </qresource>\n\
\n\
  <qresource>\n\
    <!-- Default version of a resource -->\n\
    <file>some/file</file>\n\
  </qresource>\n\
\n\
  <qresource lang=\"fr\">\n\
    <!-- French version of the same resource -->\n\
    <file alias=\"some/file\">path/to/french/version</file>\n\
  </qresource>\n\
\n\
  <qresource lang=\"fr_FR\">\n\
    <!-- Ditto, but more specialized: French from France -->\n\
    <file alias=\"some/file\">path/to/french/from/France/version</file>\n\
  </qresource>\n\
\n\
  <qresource lang=\"de\">\n\
    <!-- German version of the same resource -->\n\
    <file alias=\"some/file\">path/to/german/version</file>\n\
  </qresource>\n\
</FGRCC>\n\
\n\
Options supported by 'fgrcc'\n\
----------------------------\n\
\n\
      --root=DIR  Root directory used to interpret the real path of each\n\
                  declared resource (default: the directory containing\n\
                  INFILE)\n\
  -o, --output-cpp-file=CPP_OUT\n\
                  File where the main C++ output is to be written. If not\n\
                  specified, or if CPP_OUT is '-', the standard output is\n\
                  used.\n\
      --output-header-file=HPP_OUT\n\
                  File where to write C++ header code corresponding to the\n\
                  code in CPP_OUT (this declares the function whose name can\n\
                  be chosen with --init-func-name, see below).\n\
      --output-header-identifier=IDENT\n\
                  To avoid recursive inclusion, C and C++ header files\n\
                  are typically wrapped in a construct such as:\n\
\n\
                    #ifndef _SOME_IDENTIFIER\n\
                    #define _SOME_IDENTIFIER\n\
\n\
                    [...]\n\
\n\
                    #endif  // _SOME_IDENTIFIER\n\
\n\
                  This option allows one to choose the identifier used in\n\
                  HPP_OUT, when the --output-header-file option has been\n\
                  given.\n\
      --init-func-name=FUNC\n\
                  Name of the function declared in HPP_OUT and defined in\n\
                  CPP_OUT, that registers all resources from INFILE with the\n\
                  EmbeddedResourceManager.\n\
      --help      Display this message and exit.\n";
}

enum class ActionAfterCommandLineParsing {
  CONTINUE = 0,
  EXIT
};

struct CmdLineParams {
  SGPath rootDir;
  SGPath inputFile;
  SGPath outputCppFile;
  SGPath outputHeaderFile;
  string headerIdentifier;      // UTF-8 encoding
  string initFuncName;          // UTF-8 encoding
};

std::tuple<ActionAfterCommandLineParsing, int, CmdLineParams>
parseCommandLine(int argc, const char *const *argv)
{
  using simgear::argparse::OptionArgType;
  std::tuple<ActionAfterCommandLineParsing, int, CmdLineParams> res;
  ActionAfterCommandLineParsing& action  = std::get<0>(res);
  int& exitStatus = std::get<1>(res);
  CmdLineParams& params = std::get<2>(res);

  // Default value for the parameters
  params.rootDir = SGPath();
  params.outputCppFile = SGPath("-"); // standard output
  params.outputHeaderFile = SGPath(); // write no header file
  params.headerIdentifier = string();
  params.initFuncName = string("initEmbeddedResources");

  simgear::argparse::ArgumentParser parser;
  parser.addOption("root", OptionArgType::MANDATORY_ARGUMENT, "", "--root");
  parser.addOption("output cpp file", OptionArgType::MANDATORY_ARGUMENT,
                   "-o", "--output-cpp-file");
  parser.addOption("output header file", OptionArgType::MANDATORY_ARGUMENT,
                   "", "--output-header-file");
  parser.addOption("header identifier", OptionArgType::MANDATORY_ARGUMENT,
                   "", "--output-header-identifier");
  parser.addOption("init func name", OptionArgType::MANDATORY_ARGUMENT,
                   "", "--init-func-name");
  parser.addOption("help", OptionArgType::NO_ARGUMENT, "", "--help");

  const auto parseArgsRes = parser.parseArgs(argc, argv);

  for (const auto& opt: parseArgsRes.first) {
    if (opt.id() == "root") {
      params.rootDir = SGPath::fromUtf8(opt.value());
    } else if (opt.id() == "output cpp file") {
      params.outputCppFile = SGPath::fromUtf8(opt.value());
    } else if (opt.id() == "output header file") {
      params.outputHeaderFile = SGPath::fromUtf8(opt.value());
    } else if (opt.id() == "header identifier") {
      if (opt.value().empty()) {
        LOG("invalid empty value for option '" << opt.passedAs() << "'");
        action = ActionAfterCommandLineParsing::EXIT;
        exitStatus = EXIT_FAILURE;
        return res;
      }
      params.headerIdentifier = opt.value();
    } else if (opt.id() == "init func name") {
      params.initFuncName = opt.value();
    } else if (opt.id() == "help") {
      showUsage(cout);
      action = ActionAfterCommandLineParsing::EXIT;
      exitStatus = EXIT_SUCCESS;
      return res;
    } else {
      showUsage(cerr);
      action = ActionAfterCommandLineParsing::EXIT;
      exitStatus = EXIT_FAILURE;
      return res;
    }
  }

  if (parseArgsRes.second.size() != 1) {
    showUsage(cerr);
    action = ActionAfterCommandLineParsing::EXIT;
    exitStatus = EXIT_FAILURE;
    return res;
  }

  params.inputFile = SGPath::fromUtf8(parseArgsRes.second[0]);
  if (!params.inputFile.isFile()) {
    LOG("not an existing file: '" << params.inputFile.utf8Str() << "'");
    action = ActionAfterCommandLineParsing::EXIT;
    exitStatus = EXIT_FAILURE;
    return res;
  }

  if (!params.outputHeaderFile.isNull() && params.headerIdentifier.empty()) {
    LOG("option --output-header-identifier must be passed when "
        "--output-header-file has been given");
    action = ActionAfterCommandLineParsing::EXIT;
    exitStatus = EXIT_FAILURE;
    return res;
  }

  if (params.rootDir.isNull()) {
    params.rootDir = params.inputFile.dirPath(); // behavior of Qt's rcc
  }

  if (!params.rootDir.isDir()) {
    LOG("not an existing directory: '" << params.rootDir.utf8Str() << "'");
    action = ActionAfterCommandLineParsing::EXIT;
    exitStatus = EXIT_FAILURE;
    return res;
  }

  action = ActionAfterCommandLineParsing::CONTINUE;
  return res;
}

int doTheWork(CmdLineParams params)
{
  std::streambuf *outputStreamBuf;
  bool outputToStdout = (params.outputCppFile.utf8Str() == "-");
  SGPath outputCppFile;
  sg_ofstream output;

  if (outputToStdout) {
    // 'outputCppFile' is a null SGPath; this indicates to downstream code
    // that there is no file name/path for the generated .cxx contents.
    outputStreamBuf = cout.rdbuf();
  } else {
    outputCppFile = params.outputCppFile;
    output.open(outputCppFile);

    if (!output) {
      LOG("unable to open file '" << outputCppFile.utf8Str() << "': " <<
          simgear::strutils::error_string(errno));
      return EXIT_FAILURE;
    }

    outputStreamBuf = output.rdbuf();
  }

  std::ostream outputStream(outputStreamBuf);
  ResourceBuilderXMLVisitor xmlVisitor(params.rootDir);
  readXML(params.inputFile, xmlVisitor);
  ResourceCodeGenerator codeGenerator(xmlVisitor.getResourceDeclarations(),
                                      outputStream, outputCppFile,
                                      params.initFuncName,
                                      params.outputHeaderFile,
                                      params.headerIdentifier);
  codeGenerator.writeCode();

  return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
  int exitStatus = EXIT_FAILURE;

  std::setlocale(LC_ALL, "");
  std::setlocale(LC_NUMERIC, "C");
  std::setlocale(LC_COLLATE, "C");
  // We *might* want to call std::ios_base::sync_with_stdio(false) to maxmize
  // I/O performance, since we are neither using C's stdio stuff nor threads
  // in this program... but I haven't seen any evidence that it is needed.

  try {
    ActionAfterCommandLineParsing whatToDo;
    CmdLineParams params;
    std::tie(whatToDo, exitStatus, params) = parseCommandLine(argc, argv);

    if (whatToDo == ActionAfterCommandLineParsing::CONTINUE) {
      exitStatus = doTheWork(params);
    }
  } catch (const sg_exception &e) {
    // e.getFormattedMessage() contains the input file path specified to
    // readXML(const SGPath &path, XMLVisitor &visitor) in UTF-8 encoding.
    LOG(e.getFormattedMessage());
    LOG("aborting");
  } catch (const std::exception &e) {
    LOG(e.what());
    LOG("aborting");
  }

  return exitStatus;
}
