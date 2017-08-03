// -*- coding: utf-8 -*-
//
// fgrcc.hxx --- Simple resource compiler for FlightGear
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

#ifndef _FGRCC_HXX_
#define _FGRCC_HXX_

#include <ios>
#include <iosfwd>
#include <vector>
#include <array>
#include <cstddef>              // std::size_t

#include <simgear/misc/sg_path.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/embedded_resources/EmbeddedResource.hxx>

// Simple class to hold data gathered by the parser. Each instance corresponds
// to a resource declaration (i.e., to a 'file' element in the XML resource
// declaration file).
struct ResourceDeclaration
{
  ResourceDeclaration(
    const SGPath& virtualPath, const SGPath& realPath,
    const std::string& language,
    simgear::AbstractEmbeddedResource::CompressionType compressionType);

  bool isCompressed() const;

  SGPath virtualPath;
  SGPath realPath;
  std::string language;
  simgear::AbstractEmbeddedResource::CompressionType compressionType;
};

// Class for turning a byte stream into a stream of escape sequences suitable
// for use inside a C++ string literal. This class could be changed so as to
// be *derived* from std::istream, however this is not needed here.
class CPPEncoder
{
public:
  explicit CPPEncoder(std::istream& inputStream);

  // Used to implement operator<<
  virtual std::size_t write(std::ostream& oStream);

private:
  // errorNumber: should be the 'errno' value set by the failing call.
  [[ noreturn ]] static void handleWriteError(int errorNumber);

  std::istream& _inputStream;
};

std::ostream& operator<<(std::ostream&, CPPEncoder&);

// Generate all the C++ code needed to initialize a set of resources with the
// EmbeddedResourceManager.
class ResourceCodeGenerator
{
public:
  explicit ResourceCodeGenerator(
    const std::vector<ResourceDeclaration>& resourceDeclarations,
    std::ostream& outputStream,
    const SGPath& outputCppFile,
    const std::string& initFuncName,
    const SGPath& outputHeaderFile,
    const std::string& headerIdentifier,
    std::size_t inBufSize = 262144,
    std::size_t outBufSize = 242144);

  void writeCode() const;       // write the main C++ file
  void writeHeaderFile() const; // write the associated header file

private:
  // Return a string representing the compression type of a resource
  static std::string resourceClass(
    simgear::AbstractEmbeddedResource::CompressionType compressionType);
  // Encode an integral index in a way that can be safely used as part of a
  // C++ variable name. This is needed because, for instance, 'resource10' is
  // not a valid C++ variable name.
  static std::string encodeResourceIndex(std::size_t index);
  std::size_t writeEncodedResourceContents(const ResourceDeclaration& resDecl)
    const;

  const std::vector<ResourceDeclaration>& _resDecl;
  std::ostream& _outputStream;
  const SGPath _outputCppFile;         // SGPath() if stdout, otherwise the path
  const std::string _initFuncName;     // in UTF-8 encoding
  const SGPath _outputHeaderFile;      // SGPath() if header not requested
  const std::string _headerIdentifier; // in UTF-8 encoding

  const std::size_t _compInBufSize;
  const std::size_t _compOutBufSize;
  std::unique_ptr<char[]> _compressionInBuf;
  std::unique_ptr<char[]> _compressionOutBuf;
};

// Parser class for XML resource declaration files
class ResourceBuilderXMLVisitor: public XMLVisitor
{
public:
  explicit ResourceBuilderXMLVisitor(const SGPath& rootDir);

  // Give access to the structured data obtained once parsing is finished
  const std::vector<ResourceDeclaration>& getResourceDeclarations() const;

protected:
  void startElement(const char *name, const XMLAttributes &atts) override;
  void endElement(const char *name) override;
  void data(const char *s, int len) override;
  void warning(const char *message, int line, int column) override;

private:
  void startQResourceElement(const XMLAttributes &atts);
  void startFileElement(const XMLAttributes &atts);
  void error(const char *message, int line, int column);

  enum class XMLTagType {
    START = 0,
    END
  };

  static const std::array<std::string, 2> _tagTypeStr;

  enum class ParserState {
    START = 0,
    INSIDE_FGRCC_ELT,
    INSIDE_QRESOURCE_ELT,
    INSIDE_FILE_ELT,
    END
  };

  static const std::array<std::string, 5> _parserStateStr;

  static bool readBoolean(const std::string& s);

  static simgear::AbstractEmbeddedResource::CompressionType
  determineCompressionType(const SGPath& resourceFilePath,
                           const std::string& compression);

  [[ noreturn ]] void unexpectedTagError(
    XMLTagType tagType, const std::string& found,
    const std::string& expected = std::string());

  ParserState _parserState = ParserState::START;
  // Value obtained from the --root command line option
  const SGPath _rootDir;

  // 'prefix' attribute of the current 'qresource' element
  std::string _currentPrefix;
  // 'lang' attribute of the current 'qresource' element
  std::string _currentLanguage;
  // 'alias' attribute of the current 'file' element
  std::string _currentAlias;
  // This attribute is not part of the v1.0 QRC format, it's a FlightGear
  // extension. It tells whether, and if so, how the current <file> should be
  // compressed in the generated code.
  std::string _currentCompressionTypeStr;
  // String used to assemble resource file paths declared in 'file' elements
  // (the contents of an element can be provided in several chunks by
  // XMLVisitor::data())
  std::string _resourceFile;

  // Holds the resulting structured data once the parsing is finished
  std::vector<ResourceDeclaration> _resourceDeclarations;
};

#endif  // _FGRCC_HXX_
