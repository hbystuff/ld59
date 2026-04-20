precision highp float;
uniform sampler2D ps_texture;

varying vec2 interp_uv;
varying vec4 interp_color;

#if 0
uniform float seed;
// There's a bug that creates a spooky pattern, but
// it looks cool, so keeping the wrong version here as a reference.
const float e = 2.7182818284590452353602874713527;
float noise(vec2 uv, float seed) {
  float G = e + (seed * 0.1);
  vec2 r = (G * sin(G * uv.xy));
  return fract(r.x * r.y * (1.0 + uv.x));
}

void main(void) {
  vec4 tex_color = interp_color * texture2D(ps_texture, interp_uv);
  vec4 final_color = vec4(tex_color.rgb, tex_color.a);
  if (abs(final_color.a - 0.0) < 0.001)
    discard;

  float n = noise(interp_uv, seed);
  gl_FragColor = vec4(n,n,n, 1);
}
#else

uniform float seed;
const float e = 2.7182818284590452353602874713527;
float noise(vec2 uv, float seed) {
  float G = e + (seed * 0.1);
  vec2 r = (G * sin(G * uv.xy));
  return fract(r.x * r.y * (1.0 + uv.x));
}

void main(void) {
  vec4 tex_color = interp_color * texture2D(ps_texture, interp_uv);
  vec4 final_color = vec4(tex_color.rgb, tex_color.a);
  if (abs(final_color.a - 0.0) < 0.001)
    discard;

  float n = noise(interp_uv, seed);
  gl_FragColor = interp_color * vec4(n,n,n, 1);
}

#endif
