#ifndef SHADERS_PROJVIEW_HPP
#define SHADERS_PROJVIEW_HPP

#include "cpp_glsl_compat.h"

#define N_MAX_SHADOW_LAYERS 16

struct WorldViewProjMatrices{
    shader_mat4 mProj    [1 + N_MAX_SHADOW_LAYERS];
    shader_mat4 mView    [1 + N_MAX_SHADOW_LAYERS];
    shader_mat4 mIView   [1 + N_MAX_SHADOW_LAYERS];
    shader_mat4 mProjView[1 + N_MAX_SHADOW_LAYERS];
};


#endif /* SHADERS_PROJVIEW_HPP */
