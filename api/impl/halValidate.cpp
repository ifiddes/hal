/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.txt
 */
#include <sstream>
#include <deque>
#include "halValidate.h"
#include "hal.h"

using namespace std;
using namespace hal;

// current implementation is poor and hacky.  should fix up to 
// use iterators to properly scan the segments. 

void hal::validateBottomSegment(const BottomSegment* bottomSegment)
{
  const Genome* genome = bottomSegment->getGenome();
  hal_index_t index = bottomSegment->getArrayIndex();
  if (index < 0 || index >= (hal_index_t)genome->getSequenceLength())
  {
    stringstream ss;
    ss << "Bottom segment out of range " << index << " in genome "
       << genome->getName();
    throw hal_exception(ss.str());
  }

  hal_size_t numChildren = bottomSegment->getNumChildren();
  for (hal_size_t child = 0; child < numChildren; ++child)
  {
    const Genome* childGenome = genome->getChild(child);
    const hal_index_t childIndex = bottomSegment->getChildIndex(child);
    if (childGenome != NULL && childIndex != NULL_INDEX)
    {
      if (childIndex >= (hal_index_t)childGenome->getNumTopSegments())
      {
        stringstream ss;
        ss << "Child " << child << " index " <<childIndex << " of segment "
           << bottomSegment->getArrayIndex() << " out of range in genome "
           << childGenome->getName();
        throw hal_exception(ss.str());
      }
      const TopSegment* childSegment = 
         childGenome->getTopSegmentIterator(childIndex)->getTopSegment();
      if (childSegment->getLength() != bottomSegment->getLength())
      {
        stringstream ss;
        ss << "Child " << child << " length of segment " 
           << bottomSegment->getArrayIndex() 
           << " in genome " << genome->getName() << " has length " 
           << childSegment->getLength() << " which does not match "
           << bottomSegment->getLength();
        throw hal_exception(ss.str());
      }
    }
  }

  const hal_index_t parseIndex = bottomSegment->getTopParseIndex();
  if (parseIndex == NULL_INDEX)
  {
    if (genome->getParent() != NULL)
    {
      stringstream ss;
      ss << "Bottom segment " << bottomSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse index";
      throw hal_exception(ss.str());
    }
  }
  else
  {
    if (parseIndex >= (hal_index_t)genome->getNumTopSegments())
    {
      stringstream ss;
      ss << "Segment " << bottomSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse index";
      throw hal_exception(ss.str());
    }
    const TopSegment* parseSegment = 
       genome->getTopSegmentIterator(parseIndex)->getTopSegment();
    hal_offset_t parseOffset = bottomSegment->getTopParseOffset();
    if (parseOffset >= parseSegment->getLength())
    {
      stringstream ss;
      ss << "Segment " << bottomSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse offset";
      throw hal_exception(ss.str());
    }
  }
}

void hal::validateTopSegment(const TopSegment* topSegment)
{
  const Genome* genome = topSegment->getGenome();
  hal_index_t index = topSegment->getArrayIndex();
  if (index < 0 || index >= (hal_index_t)genome->getSequenceLength())
  {
    stringstream ss;
    ss << "Segment out of range " << index << " in genome "
       << genome->getName();
    throw hal_exception(ss.str());
  }

  const Genome* parentGenome = genome->getParent();
  const hal_index_t parentIndex = topSegment->getParentIndex();
  if (parentGenome != NULL && parentIndex != NULL_INDEX)
  {
    if (parentIndex >= (hal_index_t)parentGenome->getNumBottomSegments())
    {
      stringstream ss;
      ss << "Parent index " << parentIndex << " of segment "
         << topSegment->getArrayIndex() << " out of range in genome "
         << parentGenome->getName();
      throw hal_exception(ss.str());
    }
    const BottomSegment* parentSegment = 
       parentGenome->getBottomSegmentIterator(parentIndex)->getBottomSegment();
    if (topSegment->getLength() != parentSegment->getLength())
    {
      stringstream ss;
      ss << "Parent length of segment " << topSegment->getArrayIndex() 
         << " in genome " << genome->getName() << " has length "
         << parentSegment->getLength() << " which does not match "
         << topSegment->getLength();
      throw hal_exception(ss.str());
    }
  }

  const hal_index_t parseIndex = topSegment->getBottomParseIndex();
  if (parseIndex == NULL_INDEX)
  {
    if (genome->getNumChildren() != 0)
    {
      stringstream ss;
      ss << "Segment " << topSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse index";
      throw hal_exception(ss.str());
    }
  }
  else
  {
    if (parseIndex >= (hal_index_t)genome->getNumBottomSegments())
    {
      stringstream ss;
      ss << "Segment " << topSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse index";
      throw hal_exception(ss.str());
    }
    hal_offset_t parseOffset = topSegment->getBottomParseOffset();
    const BottomSegment* parseSegment = 
       genome->getBottomSegmentIterator(parseIndex)->getBottomSegment();
    if (parseOffset >= parseSegment->getLength())
    {
      stringstream ss;
      ss << "Segment " << topSegment->getArrayIndex() << " in genome "
         << genome->getName() << " has null parse offset";
      throw hal_exception(ss.str());
    }
  }
}

void hal::validateSequence(const Sequence* sequence)
{
  // Verify that the DNA sequence doesn't contain funny characters
  DNAIteratorConstPtr dnaIt = sequence->getDNAIterator();
  hal_size_t length = sequence->getSequenceLength();
  for (hal_size_t i = 0; i < length; ++i)
  {
    hal_dna_t c = dnaIt->getChar();
    if (isNucleotide(c) == false)
    {
      stringstream ss;
      ss << "Non-nucleotide character " << c << " discoverd at position " 
         << i << " of sequence " << sequence->getName();
      throw hal_exception(ss.str());
    }
  }
  
  // Check the top segments
  hal_size_t totalTopLength = 0;
  TopSegmentIteratorConstPtr topIt = sequence->getTopSegmentIterator();
  hal_size_t numTopSegments = sequence->getNumTopSegments();
  for (hal_size_t i = 0; i < numTopSegments; ++i)
  {
    const TopSegment* topSegment = topIt->getTopSegment();
    validateTopSegment(topSegment);
    totalTopLength += topSegment->getLength();
  }
  if (totalTopLength != length)
  {
    throw hal_exception("top lengths don't add up in " +
                        sequence->getName());
  }

  // Check the bottom segments
  hal_size_t totalBottomLength = 0;
  BottomSegmentIteratorConstPtr bottomIt = sequence->getBottomSegmentIterator();
  hal_size_t numBottomSegments = sequence->getNumBottomSegments();
  for (hal_size_t i = 0; i < numBottomSegments; ++i)
  {
    const BottomSegment* bottomSegment = bottomIt->getBottomSegment();
    validateBottomSegment(bottomSegment);
    totalBottomLength += bottomSegment->getLength();
  }
  if (totalBottomLength != length)
  {
    throw hal_exception("bottom lengths don't add up in " +
                        sequence->getName());
  }
}

void hal::validateGenome(const Genome* genome)
{
  // first we check the sequence coverage
  hal_size_t totalTop = 0;
  hal_size_t totalBottom = 0;
  hal_size_t totalLength = 0;
  
  SequenceIteratorConstPtr seqIt = genome->getSequenceIterator();
  hal_size_t numSequences = genome->getNumSequences();
  for (hal_size_t i = 0; i < numSequences; ++i)
  {
    const Sequence* sequence = seqIt->getSequence();
    validateSequence(sequence);

    totalTop += sequence->getNumTopSegments();
    totalBottom += sequence->getNumBottomSegments();
    totalLength += sequence->getSequenceLength();

    // make sure it doesn't overlap any other sequences;
    const Sequence* s1 =
       genome->getSequenceBySite(sequence->getStartPosition());
    if (s1 == NULL || s1->getName() != sequence->getName())
    {
      stringstream ss;
      ss << "Sequence " << sequence->getName() << " has a bad overlap in "
         << genome->getName();
      throw hal_exception(ss.str());
    }
    const Sequence* s2 = 
       genome->getSequenceBySite(sequence->getStartPosition() +
                                 sequence->getSequenceLength() - 1);
    if (s2 == NULL || s2->getName() != sequence->getName())
    {
      stringstream ss;
      ss << "Sequence " << sequence->getName() << " has a bad overlap in "
         << genome->getName();
      throw hal_exception(ss.str());
    }
  }

  hal_size_t genomeLength = genome->getSequenceLength();
  hal_size_t genomeTop = genome->getNumTopSegments();
  hal_size_t genomeBottom = genome->getNumBottomSegments();

  if (genomeLength != totalLength)
  {
    stringstream ss;
    ss << "Problem: genome has length " << genomeLength 
       << "But sequences total " << totalLength;
    throw hal_exception(ss.str());
  }
  if (genomeTop != totalTop)
  {
    stringstream ss;
    ss << "Problem: genome has " << genomeTop << " top segments but "
       << "sequences have " << totalTop << " top segments";
    throw ss.str();
  }
  if (genomeBottom != totalBottom)
  {
    stringstream ss;
    ss << "Problem: genome has " << genomeBottom << " bottom segments but "
       << "sequences have " << totalBottom << " bottom segments";
    throw hal_exception(ss.str());
  }
}

void hal::validateAlignment(AlignmentConstPtr alignment)
{
  deque<string> bfQueue;
  bfQueue.push_back(alignment->getRootName());
  while (bfQueue.empty() == false)
  {
    string name = bfQueue.back();
    bfQueue.pop_back();
    if (name.empty() == false)
    {
      const Genome* genome = alignment->openGenome(name);
      if (genome == NULL)
      {
        throw hal_exception("Failure to open genome " + name);
      }
      validateGenome(genome);
      vector<string> childNames = alignment->getChildNames(name);
      for (size_t i = 0; i < childNames.size(); ++i)
      {
        bfQueue.push_back(childNames[i]);
      }
    }
  }
}