import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))
from pyXPBD import *
import bpy

world = None
def load_world():
    # @todo
    world = World(WorldSettings())
load_world()

def load_post_register_world(scene):
    load_world()

def register():
    bpy.app.handlers.load_post.append(load_post_register_world)
def unregister():
    bpy.app.handlers.load_post.remove(load_post_register_world)