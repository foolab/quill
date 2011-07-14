#include "quillundocommand.h"
#include "regionsofinterest.h"

QuillMetadataRegionBag RegionsOfInterest::applyFilterToRegions(QuillImageFilter *filter,
                                                               QuillMetadataRegionBag regions)
{
    QSize size = regions.fullImageSize();
    QSize newSize = filter->newFullImageSize(size);

    QuillMetadataRegionBag result;
    result.setFullImageSize(newSize);

    foreach (QuillMetadataRegion region, regions) {
        region.setArea(filter->newArea(size, region.area()));

        if (!region.area().isEmpty())
            result.append(region);
    }
    return result;
}

QuillMetadataRegionBag
    RegionsOfInterest::applyStackToRegions(QuillUndoStack *stack,
                                           QuillMetadataRegionBag regions)
{
    int savedIndex = stack->savedIndex();
    int index = stack->index();

    if (savedIndex > index)
        return QuillMetadataRegionBag();

    for (int i=savedIndex+1; i<index; i++) {
        QuillImageFilter *filter = stack->command(i)->filter();

        regions = applyFilterToRegions(filter, regions);
    }

    return regions;
}
