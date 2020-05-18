#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

//#include "bar/baz"
//#include "c:/bar/baz"

layout(location = 0) out vec4 outColor;

//#include "foo"

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

/*
#version 400
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;
void main()
{
  outColor = color;
}
*/
