/*******************************************************************************
* Copyright 2019-2022 Intel Corporation.
*
* This software and the related documents are Intel copyrighted  materials,  and
* your use of  them is  governed by the  express license  under which  they were
* provided to you (License).  Unless the License provides otherwise, you may not
* use, modify, copy, publish, distribute,  disclose or transmit this software or
* the related documents without Intel's prior written permission.
*
* This software and the related documents  are provided as  is,  with no express
* or implied  warranties,  other  than those  that are  expressly stated  in the
* License.
*******************************************************************************/

//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

/*!
 @file Output_File.hpp

 HPCG output file classes
 */

#ifndef OUTPUTFILE_HPP
#define OUTPUTFILE_HPP

#include <list>
#include <string>

//! The OutputFile class for the uniform collecting and reporting of performance data for HPCG

/*!

  The OutputFile class facilitates easy collecting and reporting of
  key-value-formatted data that can be then registered with the HPCG results
  collection website. The keys may have hierarchy key1::key2::key3=val with
  double colon :: as a separator. A sample output may look like this (note how
  "major" and "micro" keys repeat with different ancestor keys):

\code

version=3.2.1alpha
version::major=3
version::minor=2
version::micro=1
version::release=alpha
axis=xyz
axis::major=x
axis::minor=y

\endcode

*/
class OutputFile {
protected:
  std::list<OutputFile *> descendants; //!< descendant elements
  std::string name; //!< name of the benchmark
  std::string version; //!< version of the benchmark
  std::string key; //!< the key under which the element is stored
  std::string value; //!< the value of the stored element
  std::string eol; //!< end-of-line character sequence in the output file
  std::string keySeparator; //!< character sequence to separate keys in the output file
  std::string destinationDirectory; //!< the destination directory for the generated the output file
  std::string destinationFileName; //!< the filename for the generated the output file

  //! Recursively generate output string from descendant list, and their descendants and so on
  std::string generateRecursive(std::string prefix);

public:
  static OutputFile * allocKeyVal(const std::string & key, const std::string & value);

  //! Constructor: accepts name and version as strings that are used to create a file name for printing results.
  /*!
    This constructor accepts and name and version number for the benchmark that
    are used to form a file name information for results that are generated by
    the generate() method.
    \param name (in) string containing name of the benchmark
    \param version (in) string containing the version of the benchmark
    \param destination_Directory (in, optional) path of directory where results file will be stored, relative to current working directory.
           If this value is not supplied, the results file will be stored in the current working directory.  If the directory does not exist
     it will be created.
    \param destination_FileName (in, optional) root name of the results file.  A suffix of ".yaml" will be automatically appended.  If no
           file name is specified the filename will be constructed by concatenating the miniAppName + miniAppVersion + ".yaml" strings.
  */
  OutputFile(const std::string & name, const std::string & version, const std::string & destination_Directory = "", const std::string & destination_FileName = "");

  //! Default constructor: no-arguments accepted, should be used for descendant nodes
  /*!
    This no-argument constructor can be used for descendant nodes to provide
    key1::key2::key3=val output. Unlike the root node, descendant nodes do not
    have name and version but only store key-value pairs.
  */
  OutputFile(void);

  ~OutputFile();

  //! Create and add a descendant element with value of type "string"
  /*!
  Create and add a descendant element identified by "key" and associated with
  "value".  The element is added at the end of a list of previously added
  elements.

  @param[in] key   The key that identifies the added element and under which the element is stored
  @param[in] value The value stored by the element
  */
  void add(const std::string & key, const std::string & value);

  //! Create and add a descendant element with value of type "double"
  /*!
  Create and add a descendant element identified by "key" and associated with
  "value".  The element is added at the end of a list of previously added
  elements.

  @param[in] key   The key that identifies the added element and under which the element is stored
  @param[in] value The value stored by the element
  */
   void add(const std::string & key, double value);

  //! Create and add a descendant element with value of type "int"
  /*!
  Create and add a descendant element identified by "key" and associated with
  "value".  The element is added at the end of a list of previously added
  elements.

  @param[in] key   The key that identifies the added element and under which the element is stored
  @param[in] value The value stored by the element
  */
   void add(const std::string & key, int value);

#ifndef HPCG_NO_LONG_LONG
  //! Create and add a descendant element with value of type "long long"
  /*!
  Create and add a descendant element identified by "key" and associated with
  "value".  The element is added at the end of a list of previously added
  elements.

  @param[in] key   The key that identifies the added element and under which the element is stored
  @param[in] value The value stored by the element
  */
   void add(const std::string & key, long long value);
#endif

  //! Create and add a descendant element with value of type "size_t"
  /*!
  Create and add a descendant element identified by "key" and associated with
  "value".  The element is added at the end of a list of previously added
  elements.

  @param[in] key   The key that identifies the added element and under which the element is stored
  @param[in] value The value stored by the element
  */
   void add(const std::string & key, size_t value);

  //! Key-Value setter method
  /*!
  Set the key and the value of this element.

  @param[in] key   The key that identifies this element and under which the element is stored
  @param[in] value The value stored by the element
  */
  void setKeyValue(const std::string & key, const std::string & value);

  //! Get the element in the list with the given key or return NULL if not found
  OutputFile * get(const std::string & key);

  //! Generate output string with results based on the stored key-value hierarchy
  std::string generate(void);
};

#endif // OUTPUTFILE_HPP
