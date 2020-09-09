#version 450
#pragma shader_stage(vertex)

layout(push_constant) uniform OffsetBlock { vec2 offset; } PushConstant;

// For now hard-coded, no input necessary;
vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);
void  main() {
    gl_Position = vec4(positions[gl_VertexIndex % 3] + PushConstant.offset, 0.0, 1.0);
}

/*
#version 400
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform buffer
{
  mat4 mvp;
} uniformBuffer;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;
layout (location = 0) out vec4 outColor;
void main()
{
  outColor = inColor;
  gl_Position = uniformBuffer.mvp * pos;
}
*/
