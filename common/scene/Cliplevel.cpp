#include "Cliplevel.hpp"

namespace scene::terrain {


void 
Cliplevel::allocate(glm::vec2 step_p, std::size_t resolution)
{ 
    step = step_p;
    chunk.allocate({4 * resolution, 4 * resolution});
    chunk.setExtent(4.f * step);
}
}