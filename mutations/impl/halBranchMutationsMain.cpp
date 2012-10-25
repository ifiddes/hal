/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.txt
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "halBranchMutations.h"

using namespace std;
using namespace hal;

static CLParserPtr initParser()
{
  CLParserPtr optionsParser = hdf5CLParserInstance();
  optionsParser->addArgument("halFile", "input hal file");
  optionsParser->addArgument("refGenome", 
                             "name of reference genome (analyzed branch is "
                             "this genome and its parent).");
  optionsParser->addArgument("refFile", 
                             "bed file to write structural "
                             "rearrangements in reference genome coordinates");
  optionsParser->addOption("deletionFile", 
                           "bed file to write deletions in "
                           "reference's parent genome coordinates",
                           "\"\"");
  optionsParser->addOption("snpFile", 
                           "bed file write point mutations to "
                           "in reference genome coordinates",
                           "\"\"");
  optionsParser->addOption("refSequence",
                           "name of reference sequence within reference genome"
                           " (all sequences if empty)",
                           "\"\"");
  optionsParser->addOption("start",
                           "coordinate within reference genome (or sequence"
                           " if specified) to start at",
                           0);
  optionsParser->addOption("length",
                           "length of the reference genome (or sequence"
                           " if specified) to convert.  If set to 0,"
                           " the entire thing is converted",
                           0);
  optionsParser->addOption("maxGap", 
                           "maximum indel length to be considered a gap.  Gaps "
                           " can be nested within other rearrangements.", 
                           20);
                           
  optionsParser->setDescription("Identify mutations on branch between given "
                                "genome and its parent.");
  return optionsParser;
}

int main(int argc, char** argv)
{
  CLParserPtr optionsParser = initParser();

  string halPath;
  string refBedPath;
  string delBedPath;
  string snpBedPath;
  string refGenomeName;
  string refSequenceName;
  hal_index_t start;
  hal_size_t length;
  hal_size_t maxGap;
  try
  {
    optionsParser->parseOptions(argc, argv);
    halPath = optionsParser->getArgument<string>("halFile");
    refGenomeName = optionsParser->getArgument<string>("refGenome");
    refBedPath = optionsParser->getArgument<string>("refFile");
    delBedPath = optionsParser->getOption<string>("deletionFile");
    snpBedPath = optionsParser->getOption<string>("snpFile");
    refSequenceName = optionsParser->getOption<string>("refSequence");
    start = optionsParser->getOption<hal_index_t>("start");
    length = optionsParser->getOption<hal_size_t>("length");
    maxGap = optionsParser->getOption<hal_size_t>("maxGap");
  }
  catch(exception& e)
  {
    cerr << e.what() << endl;
    optionsParser->printUsage(cerr);
    exit(1);
  }

  try
  {
    AlignmentConstPtr alignment = openHalAlignmentReadOnly(halPath);
    alignment->setOptionsFromParser(optionsParser);
    if (alignment->getNumGenomes() == 0)
    {
      throw hal_exception("hal alignmenet is empty");
    }
    
    const Genome* refGenome = NULL;
    if (refGenomeName != "\"\"")
    {
      refGenome = alignment->openGenome(refGenomeName);
      if (refGenome == NULL)
      {
        throw hal_exception(string("Reference genome, ") + refGenomeName + 
                            ", not found in alignment");
      }
    }
    else
    {
      refGenome = alignment->openGenome(alignment->getRootName());
    }
    const SegmentedSequence* ref = refGenome;
    
    const Sequence* refSequence = NULL;
    if (refSequenceName != "\"\"")
    {
      refSequence = refGenome->getSequence(refSequenceName);
      ref = refSequence;
      if (refSequence == NULL)
      {
        throw hal_exception(string("Reference sequence, ") + refSequenceName + 
                            ", not found in reference genome, " + 
                            refGenome->getName());
      }
    }

    ofstream refBedStream;
    refBedStream.open(refBedPath.c_str());
    if (!refBedStream)
    {
      throw hal_exception("Error opening " + refBedPath);
    }
  
    ofstream delBedStream;
    if (delBedPath != "\"\"")
    {
      delBedStream.open(delBedPath.c_str());
      if (!delBedStream)
      {
        throw hal_exception("Error opening " + delBedPath);
      }
    }

    ofstream snpBedStream;
    if (snpBedPath != "\"\"")
    {
      snpBedStream.open(snpBedPath.c_str());
      if (!snpBedStream)
      {
        throw hal_exception("Error opening " + snpBedPath);
      }
    }

    assert(ref != NULL);
    BranchMutations mutations;
    mutations.analyzeAlignment(alignment, maxGap, &refBedStream, &delBedStream,
                               &snpBedStream, ref, start, length);
  }
  catch(hal_exception& e)
  {
    cerr << "hal exception caught: " << e.what() << endl;
    return 1;
  }
  catch(exception& e)
  {
    cerr << "Exception caught: " << e.what() << endl;
    return 1;
  }
  
  return 0;
}