#ifndef SHADERS_PROJVIEW_HPP
#define SHADERS_PROJVIEW_HPP

#include "cpp_glsl_compat.h"

#define N_MAX_SHADOW_LAYERS 4

struct WorldViewProjMatrices{
    shader_mat4 mProj;
    shader_mat4 mView;
    shader_mat4 mIView;
    shader_mat4 mProjView[1 + N_MAX_SHADOW_LAYERS];
};


#endif /* SHADERS_PROJVIEW_HPP */
