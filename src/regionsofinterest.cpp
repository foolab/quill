#include "quillundocommand.h"
#include "regionsofinterest.h"

QuillMetadataRegionList RegionsOfInterest::applyFilterToRegions(QuillImageFilter *filter,
                                                               QuillMetadataRegionList regions)
{
    QSize size = regions.fullImageSize();
    QSize newSize = filter->newFullImageSize(size);

    QuillMetadataRegionList result;
    result.setFullImageSize(newSize);

    foreach (QuillMetadataRegion region, regions) {
        region.setArea(filter->newArea(size, region.area()));

        if (!region.area().isEmpty())
            result.append(region);
    }
    return result;
}

QuillMetadataRegionList
    RegionsOfInterest::applyStackToRegions(QuillUndoStack *stack,
                                           QuillMetadataRegionList regions)
{
    int savedIndex = stack->savedIndex();
    int index = stack->index();

    if (savedIndex >= index)
        return QuillMetadataRegionList();

    for (int i=savedIndex+1; i<index; i++) {
        QuillImageFilter *filter = stack->command(i)->filter();

        regions = applyFilterToRegions(filter, regions);
    }

    return regions;
}
