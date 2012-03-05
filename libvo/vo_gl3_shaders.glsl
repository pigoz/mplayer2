/*
 * This file is part of mplayer2.
 *
 * mplayer2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mplayer2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mplayer2; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// Note that this file is not directly passed as shader, but run through some
// text processing functions, and in fact contains multiple vertex and fragment
// shaders.

#!section prelude
// inserted at the beginning of all shaders
#version 150

#!section vertex_all
uniform mat3 transform;

in vec2 vertex_position;
in vec4 vertex_color;
out vec4 color;
in vec2 vertex_texcoord;
out vec2 texcoord;

void main() {
    vec3 position = vec3(vertex_position, 1);
#ifndef FIXED_SCALE
    position = transform * position;
#endif
    gl_Position = vec4(position, 1);
    color = vertex_color;
    texcoord = vertex_texcoord;
}

#!section frag_eosd
uniform sampler2D texture1;

in vec2 texcoord;
in vec4 color;
out vec4 out_color;

void main() {
    out_color = vec4(color.rgb, color.a * texture(texture1, texcoord).r);
}

#!section frag_osd
uniform sampler2D texture1;

in vec2 texcoord;
in vec4 color;
out vec4 out_color;

void main() {
    out_color = texture(texture1, texcoord).rrrg * color;
}

#!section frag_video
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler1D lut_c_1d;
uniform sampler1D lut_l_1d;
uniform sampler2D lut_c_2d;
uniform sampler2D lut_l_2d;
uniform sampler3D lut_3d;
uniform mat4x3 colormatrix;
uniform vec3 inv_gamma;
uniform float conv_gamma;
uniform float filter_strength;

in vec2 texcoord;
out vec4 out_color;

vec4 sample_bilinear(sampler2D tex, vec2 texcoord) {
    return texture(tex, texcoord);
}

// Explanation how bicubic scaling with only 4 texel fetches is done:
//   http://www.mate.tue.nl/mate/pdfs/10318.pdf
//   'Efficient GPU-Based Texture Interpolation using Uniform B-Splines'
// Explanation why this algorithm normally always blurs, even with unit scaling:
//   http://bigwww.epfl.ch/preprints/ruijters1001p.pdf
//   'GPU Prefilter for Accurate Cubic B-spline Interpolation'
vec4 calcweights(float s) {
    vec4 t = vec4(-0.5, 0.1666, 0.3333, -0.3333) * s + vec4(1, 0, -0.5, 0.5);
    t = t * s + vec4(0, 0, -0.5, 0.5);
    t = t * s + vec4(-0.6666, 0, 0.8333, 0.1666);
    vec2 a = vec2(1 / t.z, 1 / t.w);
    t.xy = t.xy * a + vec2(1, 1);
    t.x = t.x + s;
    t.y = t.y - s;
    return t;
}

vec4 sample_bicubic_fast(sampler2D tex, vec2 texcoord) {
    vec2 texsize = textureSize(tex, 0);
    vec2 pt = 1 / texsize;
    vec2 fcoord = fract(texcoord * texsize + vec2(0.5, 0.5));
    vec4 parmx = calcweights(fcoord.x);
    vec4 parmy = calcweights(fcoord.y);
    vec4 cdelta;
    cdelta.xz = parmx.rg * vec2(-pt.x, pt.x);
    cdelta.yw = parmy.rg * vec2(-pt.y, pt.y);
    // first y-interpolation
    vec4 ar = texture(tex, texcoord + cdelta.xy);
    vec4 ag = texture(tex, texcoord + cdelta.xw);
    vec4 ab = mix(ag, ar, parmy.b);
    // second y-interpolation
    vec4 br = texture(tex, texcoord + cdelta.zy);
    vec4 bg = texture(tex, texcoord + cdelta.zw);
    vec4 aa = mix(bg, br, parmy.b);
    // x-interpolation
    return mix(aa, ab, parmx.b);
}

float[2] weights2(sampler1D lookup, float f) {
    vec4 c = texture(lookup, f);
    return float[2](c.r, c.g);
}

float[4] weights4(sampler1D lookup, float f) {
    vec4 c = texture(lookup, f);
    return float[4](c.r, c.g, c.b, c.a);
}

float[6] weights6(sampler2D lookup, float f) {
    vec4 c1 = texture(lookup, vec2(0.25, f));
    vec4 c2 = texture(lookup, vec2(0.75, f));
    return float[6](c1.r, c1.g, c1.b, c2.r, c2.g, c2.b);
}

float[8] weights8(sampler2D lookup, float f) {
    vec4 c1 = texture(lookup, vec2(0.25, f));
    vec4 c2 = texture(lookup, vec2(0.75, f));
    return float[8](c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a);
}

float[12] weights12(sampler2D lookup, float f) {
    vec4 c1 = texture(lookup, vec2(1.0/6.0, f));
    vec4 c2 = texture(lookup, vec2(0.5, f));
    vec4 c3 = texture(lookup, vec2(5.0/6.0, f));
    return float[12](c1.r, c1.g, c1.b, c1.a,
                     c2.r, c2.g, c2.b, c2.a,
                     c3.r, c3.g, c3.b, c3.a);
}

float[16] weights16(sampler2D lookup, float f) {
    vec4 c1 = texture(lookup, vec2(0.125, f));
    vec4 c2 = texture(lookup, vec2(0.375, f));
    vec4 c3 = texture(lookup, vec2(0.625, f));
    vec4 c4 = texture(lookup, vec2(0.875, f));
    return float[16](c1.r, c1.g, c1.b, c1.a, c2.r, c2.g, c2.b, c2.a,
                     c3.r, c3.g, c3.b, c3.a, c4.r, c4.g, c4.b, c4.a);
}

#define CONVOLUTION_SEP_N(NAME, N)                                           \
    vec4 NAME(sampler2D tex, vec2 texcoord, vec2 pt, float weights[N]) {     \
        vec4 res = vec4(0);                                                  \
        for (int n = 0; n < N; n++) {                                        \
            res += weights[n] * texture(tex, texcoord + pt * n);             \
        }                                                                    \
        return res;                                                          \
    }

CONVOLUTION_SEP_N(convolution_sep2, 2)
CONVOLUTION_SEP_N(convolution_sep4, 4)
CONVOLUTION_SEP_N(convolution_sep6, 6)
CONVOLUTION_SEP_N(convolution_sep8, 8)
CONVOLUTION_SEP_N(convolution_sep12, 12)
CONVOLUTION_SEP_N(convolution_sep16, 16)

// The dir parameter is (0, 1) or (1, 0), and we expect the shader compiler to
// remove all the redundant multiplications and additions.
#define SAMPLE_CONVOLUTION_SEP_N(NAME, N, SAMPLERT, CONV_FUNC, WEIGHTS_FUNC)\
    vec4 NAME(vec2 dir, SAMPLERT lookup, sampler2D tex, vec2 texcoord) {    \
        vec2 texsize = textureSize(tex, 0);                                 \
        vec2 pt = (1 / texsize) * dir;                                      \
        float fcoord = dot(fract(texcoord * texsize - 0.5), dir);           \
        vec2 base = texcoord - fcoord * pt;                                 \
        return CONV_FUNC(tex, base - pt * (N / 2 - 1), pt,                  \
                         WEIGHTS_FUNC(lookup, fcoord));                     \
    }

SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep2, 2, sampler1D, convolution_sep2, weights2)
SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep4, 4, sampler1D, convolution_sep4, weights4)
SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep6, 6, sampler2D, convolution_sep6, weights6)
SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep8, 8, sampler2D, convolution_sep8, weights8)
SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep12, 12, sampler2D, convolution_sep12, weights12)
SAMPLE_CONVOLUTION_SEP_N(sample_convolution_sep16, 16, sampler2D, convolution_sep16, weights16)


#define CONVOLUTION_N(NAME, N)                                               \
    vec4 NAME(sampler2D tex, vec2 texcoord, vec2 pt, float taps_x[N],        \
              float taps_y[N]) {                                             \
        vec4 res = vec4(0);                                                  \
        for (int y = 0; y < N; y++) {                                        \
            vec4 line = vec4(0);                                             \
            for (int x = 0; x < N; x++)                                      \
                line += taps_x[x] * texture(tex, texcoord + pt * vec2(x, y));\
            res += taps_y[y] * line;                                         \
        }                                                                    \
        return res;                                                          \
    }

CONVOLUTION_N(convolution2, 2)
CONVOLUTION_N(convolution4, 4)
CONVOLUTION_N(convolution6, 6)
CONVOLUTION_N(convolution8, 8)
CONVOLUTION_N(convolution12, 12)
CONVOLUTION_N(convolution16, 16)

#define SAMPLE_CONVOLUTION_N(NAME, N, SAMPLERT, CONV_FUNC, WEIGHTS_FUNC)    \
    vec4 NAME(SAMPLERT lookup, sampler2D tex, vec2 texcoord) {              \
        vec2 texsize = textureSize(tex, 0);                                 \
        vec2 pt = 1 / texsize;                                              \
        vec2 fcoord = fract(texcoord * texsize - 0.5);                      \
        vec2 base = texcoord - fcoord * pt;                                 \
        return CONV_FUNC(tex, base - pt * (N / 2 - 1), pt,                  \
                         WEIGHTS_FUNC(lookup, fcoord.x),                    \
                         WEIGHTS_FUNC(lookup, fcoord.y));                   \
    }

SAMPLE_CONVOLUTION_N(sample_convolution2, 2, sampler1D, convolution2, weights2)
SAMPLE_CONVOLUTION_N(sample_convolution4, 4, sampler1D, convolution4, weights4)
SAMPLE_CONVOLUTION_N(sample_convolution6, 6, sampler2D, convolution6, weights6)
SAMPLE_CONVOLUTION_N(sample_convolution8, 8, sampler2D, convolution8, weights8)
SAMPLE_CONVOLUTION_N(sample_convolution12, 12, sampler2D, convolution12, weights12)
SAMPLE_CONVOLUTION_N(sample_convolution16, 16, sampler2D, convolution16, weights16)


// Unsharp masking
vec4 sample_sharpen3(sampler2D tex, vec2 texcoord) {
    vec2 texsize = textureSize(tex, 0);
    vec2 pt = 1 / texsize;
    vec2 st = pt * 0.5;
    vec4 p = texture(tex, texcoord);
    vec4 sum = texture(tex, texcoord + st * vec2(+1, +1))
             + texture(tex, texcoord + st * vec2(+1, -1))
             + texture(tex, texcoord + st * vec2(-1, +1))
             + texture(tex, texcoord + st * vec2(-1, -1));
    return p + (p - 0.25 * sum) * filter_strength;
}

vec4 sample_sharpen5(sampler2D tex, vec2 texcoord) {
    vec2 texsize = textureSize(tex, 0);
    vec2 pt = 1 / texsize;
    vec2 st1 = pt * 1.2;
    vec4 p = texture(tex, texcoord);
    vec4 sum1 = texture(tex, texcoord + st1 * vec2(+1, +1))
              + texture(tex, texcoord + st1 * vec2(+1, -1))
              + texture(tex, texcoord + st1 * vec2(-1, +1))
              + texture(tex, texcoord + st1 * vec2(-1, -1));
    vec2 st2 = pt * 1.5;
    vec4 sum2 = texture(tex, texcoord + st2 * vec2(+1,  0))
              + texture(tex, texcoord + st2 * vec2( 0, +1))
              + texture(tex, texcoord + st2 * vec2(-1,  0))
              + texture(tex, texcoord + st2 * vec2( 0, -1));
    vec4 t = p * 0.859375 + sum2 * -0.1171875 + sum1 * -0.09765625;
    return p + t * filter_strength;
}

void main() {
#ifdef USE_PLANAR
    vec3 color = vec3(SAMPLE_L(texture1, texcoord).r,
                      SAMPLE_C(texture2, texcoord).r,
                      SAMPLE_C(texture3, texcoord).r);
#else
    vec3 color = SAMPLE_L(texture1, texcoord).rgb;
#endif
#ifdef USE_YGRAY
    color.gb = vec2(0.5);
#endif
#ifdef USE_COLORMATRIX
    color = mat3(colormatrix) * color + colormatrix[3];
#endif
#ifdef USE_LINEAR_CONV
    color = pow(color, vec3(2.2));
#endif
#ifdef USE_LINEAR_CONV_INV
    // Convert from linear RGB to gamma RGB before putting it through the 3D-LUT
    // in the final stage.
    color = pow(color, vec3(1.0/2.2));
#endif
#ifdef USE_GAMMA_POW
    color = pow(color, inv_gamma);
#endif
#ifdef USE_3DLUT
    color = texture(lut_3d, color).rgb;
#endif
    out_color = vec4(color, 1);
}

