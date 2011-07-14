#ifndef REGIONS_OF_INTEREST_H
#define REGIONS_OF_INTEREST_H

#include <QuillImageFilter>
#include <quillmetadata/QuillMetadata>
#include <quillmetadata/QuillMetadataRegion>
#include "quillundostack.h"

class RegionsOfInterest {

    friend class ut_regions;

 public:

    /*!
      Applies one filter to a region bag
    */

    static QuillMetadataRegionBag applyFilterToRegions(QuillImageFilter *filter,
                                                       QuillMetadataRegionBag regions);

    /*!
      Applies a whole stack of filters to a QuillMetadata object
    */

    static QuillMetadataRegionBag applyStackToRegions(QuillUndoStack *stack,
                                                      QuillMetadataRegionBag regions);
};

#endif // REGIONS_OF_INTEREST_H
