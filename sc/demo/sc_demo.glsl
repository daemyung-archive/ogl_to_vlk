precision mediump float;

layout(location = 0) in vec4 vertex_color0;
layout(location = 1) in vec4 vertex_color1;
layout(location = 2) in vec4 vertex_color2;
layout(location = 3) in vec2 vertex_tex_coord;

layout(location = 0) out vec4 frag_color0;
layout(location = 1) out vec4 frag_color1;
layout(location = 2) out vec4 frag_color2;
layout(location = 3) out vec4 frag_color3;

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout(binding = 0) uniform Colors {
	vec4 color0;
	vec4 color1;
	vec4 color2;
};

layout(binding = 1) uniform Geometry {
	vec4 translate;
	vec4 rotate;
	vec4 scale;
};

layout(binding = 2) uniform Materials {
	Material mirror;
	Material rubber;
};

layout(binding = 0) uniform sampler2D base_texture0;
layout(binding = 1) uniform sampler2D base_texture1;
layout(binding = 2) uniform sampler2D base_texture2;

void main()
{
	vec4 base_color0 = texture(base_texture0, vertex_tex_coord);
	vec4 base_color1 = texture(base_texture1, vertex_tex_coord);
	vec4 base_color2 = texture(base_texture2, vertex_tex_coord);

	frag_color0 = mirror.diffuse * vertex_color0 * base_color0 * color0;
	frag_color1 = rubber.diffuse * vertex_color1 * base_color1 * color1;
	frag_color2 = vertex_color2 * base_color2 * color2;
	frag_color3 = translate * rotate * scale * color0;
}