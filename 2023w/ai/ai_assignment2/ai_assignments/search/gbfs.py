import math
from .. problem import Problem
from .. datastructures.priority_queue import PriorityQueue


# please ignore this
def get_solver_mapping():
    return dict(
        gbfs_mh=GBFS_Manhattan,
        gbfs_ch=GBFS_Chebyshev
    )


class GBFS(object):
    # TODO, Exercise 2:
    # - implement Greedy Best First Search (GBFS)
    # - use the provided PriorityQueue where appropriate
    # - to put items into the PriorityQueue, use 'pq.put(<priority>, <item>)'
    # - to get items out of the PriorityQueue, use 'pq.get()'
    # - use a 'set()' to store nodes that were already visited
    def solve(self, problem: Problem):
        return None


# please note that in an ideal world, the heuristics should actually be part
# of the problem definition, as it assumes domain knowledge about the structure
# of the problem, and defines a distance to the goal state


# this is the GBFS variant with the manhattan distance as a heuristic
# it is registered as a solver with the name 'gbfs_mh'
class GBFS_Manhattan(GBFS):
    def heuristic(self, current, goal):
        cy, cx = current.state
        gy, gx = goal.state
        return math.fabs((cy - gy)) + math.fabs(cx - gx)


# this is the GBFS variant with the chebyshev distance as a heuristic
# it is registered as a solver with the name 'gbfs_ch'
class GBFS_Chebyshev(GBFS):
    def heuristic(self, current, goal):
        # TODO, Exercise 2:
        # implement Chebyshev distance (see slides)
        # - get x- and y-coordinates for current and goal node (see Manhattan distance)
        # - compute Chebyshev distances based on formula in slides: max_i(|a_i - b_i|)
        # return Chebyshev distance
        return None
