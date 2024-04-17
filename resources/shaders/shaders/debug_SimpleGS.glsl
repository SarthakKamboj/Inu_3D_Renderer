#version 410 core

#define NUM_CASCADES 3

layout (triangles, invocations = NUM_CASCADES) in;
layout (triangles_strip, max_vertices = 3) out;

uniform mat4 light_views[NUM_CASCADES];
uniform mat4 light_projs[NUM_CASCADES];

void main() {
  for (int i = 0; i < 3; i++) {
    gl_Position = light_projs[gl_InvocationID] * light_views[gl_InvocationID] * gl_in[i];
    gl_Layer = gl_InvocationID;
    EmitVertex();
  }
  EmitPrimitive();
}
