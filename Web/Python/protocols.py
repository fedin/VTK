r"""protocols is a module that contains a set of VTK Web related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

from time import time
import inspect
import logging
import sys
import traceback
import types

# import RPC annotation
from autobahn.wamp import exportRpc

from vtkWebCorePython import vtkWebApplication, vtkWebInteractionEvent

# =============================================================================
#
# Base class for any VTK Web based protocol
#
# =============================================================================

class vtkWebProtocol(object):
    def setApplication(self, app):
        self.Application = app

    def getApplication(self):
        return self.Application

    def mapIdToObject(self, id):
        """
        Maps global-id for a vtkObject to the vtkObject instance. May return None if the
        id is not valid.
        """
        id = int(id)
        if id <= 0:
            return None
        return self.Application.GetObjectIdMap().GetVTKObject(id)

    def getGlobalId(self, obj):
        """
        Return the id for a given vtkObject
        """
        return self.Application.GetObjectIdMap().GetGlobalId(obj)

    def getView(self, vid):
        """
        Returns the view for a given view ID, if vid is None then return the
        current active view.
        :param vid: The view ID
        :type vid: str
        """
        view = self.mapIdToObject(vid)

        if not view:
            # Use active view is none provided.
            view = self.Application.GetObjectIdMap().GetActiveObject("VIEW")
        if not view:
            raise Exception("no view provided: " + vid)

        return view

    def setActiveView(self, view):
        """
        Set a vtkRenderWindow to be the active one
        """
        self.Application.GetObjectIdMap().SetActiveObject("VIEW", view)


# =============================================================================
#
# Handle Mouse interaction on any type of view
#
# =============================================================================

class vtkWebMouseHandler(vtkWebProtocol):

    @exportRpc("mouseInteraction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkWebInteractionEvent.LEFT_BUTTON;
        if event["buttonMiddle"]:
            buttons |= vtkWebInteractionEvent.MIDDLE_BUTTON;
        if event["buttonRight"]:
            buttons |= vtkWebInteractionEvent.RIGHT_BUTTON;

        modifiers = 0
        if event["shiftKey"]:
            modifiers |= vtkWebInteractionEvent.SHIFT_KEY
        if event["ctrlKey"]:
            modifiers |= vtkWebInteractionEvent.CTRL_KEY
        if event["altKey"]:
            modifiers |= vtkWebInteractionEvent.ALT_KEY
        if event["metaKey"]:
            modifiers |= vtkWebInteractionEvent.META_KEY

        pvevent = vtkWebInteractionEvent()
        pvevent.SetButtons(buttons)
        pvevent.SetModifiers(modifiers)
        pvevent.SetX(event["x"])
        pvevent.SetY(event["y"])
        if event["action"] == 'dblclick':
             pvevent.SetRepeatCount(2)
        #pvevent.SetKeyCode(event["charCode"])
        retVal = self.getApplication().HandleInteractionEvent(view, pvevent)
        del pvevent
        return retVal

# =============================================================================
#
# Basic 3D Viewport API (Camera + Orientation + CenterOfRotation
#
# =============================================================================

class vtkWebViewPort(vtkWebProtocol):

    @exportRpc("resetCamera")
    def resetCamera(self, view):
        """
        RPC callback to reset camera.
        """
        view = self.getView(view)
        camera = view.GetRenderer().GetActiveCamera()
        camera.ResetCamera()
        try:
            # FIXME seb: view.CenterOfRotation = camera.GetFocalPoint()
            print "FIXME"
        except:
            pass

        self.getApplication().InvalidateCache(view)
        return str(self.getGlobalId(view))

    @exportRpc("updateOrientationAxesVisibility")
    def updateOrientationAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(view)
        # FIXME seb: view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        return str(self.getGlobalId(view))

    @exportRpc("updateCenterAxesVisibility")
    def updateCenterAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(view)
        # FIXME seb: view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view)
        return str(self.getGlobalId(view))

    @exportRpc("updateCamera")
    def updateCamera(self, view_id, focal_point, view_up, position):
        view = self.getView(view_id)

        camera = view.GetRenderer().GetActiveCamera()
        camera.SetFocalPoint(focal_point)
        camera.SetCameraViewUp(view_up)
        camera.SetCameraPosition(position)
        self.getApplication().InvalidateCache(view)

# =============================================================================
#
# Provide Image delivery mechanism
#
# =============================================================================

class vtkWebViewPortImageDelivery(vtkWebProtocol):

    @exportRpc("stillRender")
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time() * 1000))
        view = self.getView(options["view"])
        size = [view.GetSize()[0], view.GetSize()[1]]
        if options and options.has_key("size"):
            size = options["size"]
            view.SetSize(size)
        t = 0
        if options and options.has_key("mtime"):
            t = options["mtime"]
        quality = 100
        if options and options.has_key("quality"):
            quality = options["quality"]
        localTime = 0
        if options and options.has_key("localTime"):
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        if t == 0:
            app.InvalidateCache(view)
        reply["image"] = app.StillRenderToString(view, t, quality)
        reply["stale"] = app.GetHasImagesBeingProcessed(view)
        reply["mtime"] = app.GetLastStillRenderToStringMTime()
        reply["size"] = [view.GetSize()[0], view.GetSize()[1]]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = str(self.getGlobalId(view))
        reply["localTime"] = localTime

        endTime = int(round(time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply


# =============================================================================
#
# Provide Geometry delivery mechanism (WebGL)
#
# =============================================================================

class vtkWebViewPortGeometryDelivery(vtkWebProtocol):

    @exportRpc("getSceneMetaData")
    def getSceneMetaData(self, view_id):
        view  = self.getView(view_id);
        data = self.getApplication().GetWebGLSceneMetaData(view)
        return data

    @exportRpc("getWebGLData")
    def getWebGLData(self, view_id, object_id, part):
        view  = self.getView(view_id)
        data = self.getApplication().GetWebGLBinaryData(view, str(object_id), part-1)
        return data
