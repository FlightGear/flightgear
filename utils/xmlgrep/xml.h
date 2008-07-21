/* Copyright (c) 2007, 2008 by Adalin B.V.
 * Copyright (c) 2007, 2008 by Erik Hofman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of (any of) the copyrightholder(s) nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
 * THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __XML_CONFIG
#define __XML_CONFIG 1

/**
 * Open an XML file for processing
 *
 * @param fname path to the file 
 * @return XML-id which is used for further processing
 */
void *xmlOpen(const char *);

/**
 * Close the XML file after which no further processing is possible
 *
 * @param xid XML-id
 */
void xmlClose(void *);


/**
 * Locate a subsection of the xml tree for further processing.
 * This adds processing speed since the reuired nodes will only be searched
 * in the subsection
 *
 * The memory allocated for the XML-subsection-id has to be freed by the
 * calling process
 *
 * @param xid XML-id
 * @param node path to the node containing the subsection
 * @return XML-subsection-id for further processing
 */
void *xmlGetNode(const void *, const char *);

/**
 * Copy a subsection of the xml tree for further processing.
 * This is useful when it's required to process a section of the XML code
 * after the file has been closed. The drawback is the added memory
 * requirements
 *
 * The memory allocated for the XML-subsection-id has to be freed by the
 * calling process.
 *
 * @param xid XML-id
 * @param node path to the node containing the subsection
 * @return XML-subsection-id for further processing
 */
void *xmlCopyNode(const void *, const char *);

/**
 * Return the name of this node.
 * The returned string has to be freed by the calling process
 *
 * @param xid XML-id
 * @return a newly alocated string containing the node name
 */
char *xmlGetNodeName(const void *);

/**
 * Copy the name of this node in a pre-allocated buffer
 *
 * @param xid XML-id
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the node name
 */
size_t xmlCopyNodeName(const void *, const char *, size_t);

/**
 * Get the number of nodes with the same name from a specified xml path
 *
 * @param xid XML-id
 * @param path path to the xml node
 * @return the number count of the nodename
 */
unsigned int xmlGetNumNodes(const void *, const char *);

/**
 * Get the nth occurrence of node in the parent node.
 * The return value should neevr be altered or freed by the caller
 *
 * @param pid XML-id of the parent node of this node
 * @param xid XML-id
 * @param node name of the node to search for
 * @param num specify which occurence to return
 * @return XML-subsection-id for further processing or NULL if unsuccessful
 */
void *xmlGetNodeNum(const void *, void *, const char *, int);

/**
 * Compare the value of this node to a reference string.
 * Comparing is done in a case insensitive way
 *
 * @param xid XML-id
 * @param str the string to compare to
 * @return an integer less than, equal to, ro greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
int xmlCompareString(const void *, const char *);


/**
 * Get a string of characters from a specified xml path.
 * This function has the advantage of not allocating its own return buffer,
 * keeping the memory management to an absolute minimum but the disadvantage
 * is that it's unreliable in multithread environments
 *
 * @param xid XML-id
 * @param path path to the xml node
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the string
 */
size_t xmlCopyNodeString(const void *, const char *, char *, size_t);

/**
 * Get a string of characters from the current node.
 * The returned string has to be freed by the calling process
 *
 * @param xid XML-id
 * @return a newly alocated string containing the contents of the node
 */
char *xmlGetString(const void *);

/**
 * Get a string of characters from a specified xml path.
 * The returned string has to be freed by the calling process
 *
 * @param xid XML-id
 * @param path path to the xml node
 * @return a newly alocated string containing the contents of the node
 */
char *xmlGetNodeString(const void *, const char *);

/**
 * Get a string of characters from the current node.
 * This function has the advantage of not allocating its own return buffer,
 * keeping the memory management to an absolute minimum but the disadvantage
 * is that it's unreliable in multithread environments
 *
 * @param xid XML-id
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the string
 */
size_t xmlCopyString(const void *, char *, size_t);

/**
 * Compare the value of a node to a reference string.
 * Comparing is done in a case insensitive way
 *
 * @param xid XML-id
 * @param path path to the xml node to compare to
 * @param str the string to compare to
 * @return an integer less than, equal to, ro greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
int xmlCompareNodeString(const void *, const char *, const char *);

/**
 * Get the integer value from the current node
 *
 * @param xid XML-id
 * @return the contents of the node converted to an integer value
 */
long int xmlGetInt(const void *);

/**
 * Get an integer value from a specified xml path
 *
 * @param xid XML-id
 * @param path path to the xml node
 * @return the contents of the node converted to an integer value
 */
long int xmlGetNodeInt(const void *, const char *);

/**
 * Get the double value from the curent node
 *
 * @param xid XML-id
 * @return the contents of the node converted to a double value
 */
double xmlGetDouble(const void *);

/**
 * Get a double value from a specified xml path
 *
 * @param xid XML-id
 * @param path path to the xml node
 * @return the contents of the node converted to a double value
 */
double xmlGetNodeDouble(const void *, const char *);

/**
 * Create a marker XML-id that starts out with the same settings as the
 * refference XML-id.
 * The returned XML-id has to be freed by the calling process
 *
 * @param xid reference XML-id
 * @return a copy of the reference XML-id
 */
void *xmlMarkId(const void *);

#endif /* __XML_CONFIG */

