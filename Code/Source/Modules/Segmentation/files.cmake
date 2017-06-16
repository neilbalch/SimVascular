set(H_FILES
    svSegmentationUtils.h \
    svContour.h \
    svContourCircle.h \
    svContourEllipse.h \
    svContourPolygon.h \
    svContourTensionPolygon.h \
    svContourSplinePolygon.h \
    svContourOperation.h \
    svContourModel.h \
    svContourModelVtkMapper2D.h \
    svContourModelThresholdInteractor.h \
    svContourGroup.h \
    svContourGroupVtkMapper2D.h \
    svContourGroupVtkMapper3D.h \
    svContourGroupDataInteractor.h \
    svContourGroupIO.h \
    svSegmentationLegacyIO.h \
    svSurface.h \
    svSurfaceVtkMapper3D.h \
    svSeg3D.h \
    svMitkSeg3D.h \
    svMitkSeg3DOperation.h \
    svMitkSeg3DIO.h \
    svMitkSeg3DVtkMapper3D.h \
    svMitkSeg3DDataInteractor.h \
    svSegmentationObjectFactory.h \
    svSeg3DUtils.h
)

set(CPP_FILES
    svSegmentationUtils.cxx \
    svContour.cxx \
    svContourCircle.cxx \
    svContourEllipse.cxx \
    svContourPolygon.cxx \
    svContourTensionPolygon.cxx \
    svContourSplinePolygon.cxx \
    svContourOperation.cxx \
    svContourModel.cxx \
    svContourModelVtkMapper2D.cxx \
    svContourModelThresholdInteractor.cxx \
    svContourGroup.cxx \
    svContourGroupVtkMapper2D.cxx \
    svContourGroupVtkMapper3D.cxx \
    svContourGroupDataInteractor.cxx \
    svContourGroupIO.cxx \
    svSegmentationLegacyIO.cxx \
    svSurface.cxx \
    svSurfaceVtkMapper3D.cxx \
    svSeg3D.cxx \
    svMitkSeg3D.cxx \
    svMitkSeg3DOperation.cxx \
    svMitkSeg3DIO.cxx \
    svMitkSeg3DVtkMapper3D.cxx \
    svMitkSeg3DDataInteractor.cxx \
    svSegmentationObjectFactory.cxx \
    svSeg3DUtils.cxx
)

set(RESOURCE_FILES
    Interactions/svContourGroupInteraction.xml \
    Interactions/svContourModelThresholdInteraction.xml \
    Interactions/svMitkSeg3DInteraction.xml \
    Interactions/svSegmentationConfig.xml
)
