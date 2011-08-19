#ifndef REGIONS_OF_INTEREST_H
#define REGIONS_OF_INTEREST_H

#include <QuillImageFilter>
#include <quillmetadata/QuillMetadata>
#include <quillmetadata/QuillMetadataRegion>
#include <quillmetadata/QuillMetadataRegionList>
#include "quillundostack.h"

class RegionsOfInterest {

    friend class ut_regions;

 public:

    /*!
      Applies one filter to a region bag
    */

    static QuillMetadataRegionList applyFilterToRegions(QuillImageFilter *filter,
                                                        QuillMetadataRegionList regions);

    /*!
      Applies a whole stack of filters to a QuillMetadata object
    */

    static QuillMetadataRegionList applyStackToRegions(QuillUndoStack *stack,
                                                       QuillMetadataRegionList regions);
};

#endif // REGIONS_OF_INTEREST_H
