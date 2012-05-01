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
 * with mplayer2.  If not, see <http://www.gnu.org/licenses/>.
 */

struct mp_image;
struct mp_csp_details;

struct image_writer_opts {
    char *filetype;
    int png_compression;
    int jpeg_quality;
};

extern const struct image_writer_opts image_writer_opts_defaults;

extern const struct m_sub_options image_writer_conf;

// Return the file extension that will be used, e.g. "png".
const char *image_writer_file_ext(const struct image_writer_opts *opts);

/*
 * Save the given image under the given filename. The parameters csp and opts
 * are optional. All pixel formats supported by swscale are supported.
 *
 * File format and compression settings are controlled via the opts parameter.
 *
 * NOTE: The fields w/h/width/height of the passed mp_image must be all set
 *       accordingly. Setting w and width or h and height to different values
 *       can be used to store snapshots of anamorphic video.
 */
int write_image(struct mp_image *image, const struct mp_csp_details *csp,
                const struct image_writer_opts *opts, const char *filename);
