import sys, os
import numpy as np
from pyXPBD import *

# 
# setup world
# 
world = World(WorldSettings())

b1 = world.add_body()
b1.set_no_gravity(True)

b2 = world.add_body()
b2.set_inv_mass(1.0)
b2.set_position(vec3(np.array([1,0,0], dtype=np.float32)))

c1 = world.add_distance_constraint(b1, b2)
c1.set_d(1.0)

# 
# animation loop
# 
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

fig, ax = plt.subplots()
dot, = ax.plot([], [], 'bo')
ax.set_xlim(-2, 2)
ax.set_ylim(-2, 2)

def update(frame):
    world.step(1/60)
    pos = np.array(b2.get_position(), copy=False)
    dot.set_data([pos[0]], [pos[1]])
    return dot,

ani = FuncAnimation(fig, update, frames=60*5, interval=1000/60, blit=True)
plt.show()