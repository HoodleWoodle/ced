#include "ui.h"

#include <zwerg.h>

// ##################################################################
// defines
// ##################################################################

#define COLOR_U8(r, g, b) ((color_t){ (r/255.0f), (g/255.0f), (b/255.0f), 1.0f })

namespace ced::ui {

    // ##################################################################
    // types
    // ##################################################################

    typedef struct {
        float left;
        float top;
        float right;
        float bottom;
    } rect_offset_t;

    typedef struct {
        color_t color;
    } cfg_bg_t;

    typedef struct {
        float thickness;
        color_t color;
    } cfg_frame_t;

    typedef struct {
        cfg_bg_t bg;
        cfg_frame_t frame;
        rect_offset_t padding;
    } cfg_section_t;

    typedef struct {
        font_t* font;
        color_t color;
        float line_spacing;

        u64 tab_space_count;
    } cfg_text_t;

    typedef struct {
        cfg_section_t main;
        cfg_section_t left;
        cfg_text_t text;

        bool left_is_enabled;
    } cfg_panel_t;

    // ##################################################################
    // function declarations (static)
    // ##################################################################

    static void cfg_panel_init_default(cfg_panel_t* cfg);
    
    static bool get_next_draw_line(doc::iter_t* it, doc::slice_t* slice, const char** slice_progress, char* buf, u64 buf_len, const cfg_panel_t* cfg);

    // ##################################################################
    // constants
    // ##################################################################

    static const char* FONT_CHARACTERS_VISIBLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    static const char* FONT_CHARACTERS_NUM = "0123456789";

    // ##################################################################
    // globals
    // ##################################################################

    static font_t s_font_default;
    static cfg_panel_t s_cfg_panel_default;

    // ##################################################################
    // function definitions
    // ##################################################################

    void init() {
        dze_window_create("ced", 1280, 720);
        dze_qrenderer_init(10000);

	    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); GL_CHECK();

        // TODO: IMPL: font lookup
        static const char* FONT_DEFAULT_FILE = "fonts/IBM_Plex_Mono/IBMPlexMono-Regular.ttf";
        static const u64   FONT_DEFAULT_SIZE = 18;
	    dze_font_init(&s_font_default, FONT_DEFAULT_FILE, FONT_DEFAULT_SIZE);

        cfg_panel_init_default(&s_cfg_panel_default);
    }

    void deinit() {
        dze_font_free(&s_font_default);
        dze_qrenderer_cleanup();
        dze_window_destroy();
    }

    bool is_closed() {
        return dze_window_is_close_requested();
    }

    void frame_begin() {
		size2_t viewport_size = dze_window_framebuffer_size();
		glViewport(0, 0, (GLsizei)viewport_size.x, (GLsizei)viewport_size.y); GL_CHECK();

		glClear(GL_COLOR_BUFFER_BIT); GL_CHECK();
		dze_qrenderer_begin_p(viewport_size, GLMS_MAT4_IDENTITY);
    }

    void frame_end() {
		dze_qrenderer_end();
		dze_window_update();
    }

    void panel(doc::iter_t* it) {
        pos2_t pos = (pos2_t){ 0.0f, 0.0f };
		size2_t size = dze_window_framebuffer_size();
        const cfg_panel_t* cfg = &s_cfg_panel_default;

        char buf[1024];

        float font_size_y = dze_font_sizeof(cfg->text.font, FONT_CHARACTERS_VISIBLE).y;
        float font_num_size_x_max = 1.0f;
        buf[1] = '\0';
        for (const char* c = FONT_CHARACTERS_NUM; *c != '\0'; c++) {
            buf[0] = *c;
            float num_size_x = dze_font_sizeof(cfg->text.font, buf).x;
            if (num_size_x > font_num_size_x_max)
                font_num_size_x_max = num_size_x;
        }

        float tm_size_y = size.y;
        u64 draw_line_num_max = (tm_size_y - 2 * cfg->main.frame.thickness) / (font_size_y + cfg->text.line_spacing);
        u64 line_num = 1;
        u64 line_num_max = line_num + draw_line_num_max - 1;

        u64 line_num_max_digits_max = 1;
        if (line_num_max >= 100000000) { line_num_max_digits_max += 8; line_num_max /= 100000000; }
        if (line_num_max >= 10000) { line_num_max_digits_max += 4; line_num_max /= 10000; }
        if (line_num_max >= 100) { line_num_max_digits_max += 2; line_num_max /= 100; }
        if (line_num_max >= 10) { line_num_max_digits_max += 1; }
        
        // left
        qtransform_t tl = {0};
        if (cfg->left_is_enabled) {
            tl.p = pos;
            tl.s = (size2_t){
                2 * cfg->left.frame.thickness + cfg->left.padding.left + cfg->left.padding.right + line_num_max_digits_max * font_num_size_x_max,
                size.y
            };
            dze_qrenderer_draw_quad(tl, cfg->left.bg.color);
            dze_qrenderer_draw_frame(tl, cfg->left.frame.thickness, cfg->left.frame.color);
        }

        // main
        qtransform_t tm = {0};
        tm.p = (pos2_t){ tl.p.x + tl.s.x, pos.y };
        tm.s = (size2_t){ size.x - tl.s.x, tm_size_y };
        dze_qrenderer_draw_quad(tm, cfg->main.bg.color);
        dze_qrenderer_draw_frame(tm, cfg->main.frame.thickness, cfg->main.frame.color);

        // text
        u64 draw_line_idx = 0;
        doc::slice_t slice = {0};
        const char* slice_progress = NULL;
        while (get_next_draw_line(it, &slice, &slice_progress, buf, sizeof(buf), cfg)) {
            if (cfg->left_is_enabled) {
                // text - left
                char line_num_buf[21];
                sprintf(line_num_buf, "%lu", line_num);
                float line_num_buf_size_x = dze_font_sizeof(cfg->text.font, line_num_buf).x;

                qtransform_t ttl = {0};
                ttl.p = (pos2_t){
                    tl.p.x + tl.s.x - cfg->left.frame.thickness - cfg->left.padding.right - line_num_buf_size_x,
                    tl.p.y + cfg->left.frame.thickness + cfg->left.padding.top + draw_line_idx * (cfg->text.line_spacing + font_size_y)
                };
                dze_qrenderer_draw_text(cfg->text.font, ttl.p, ttl.r, line_num_buf, cfg->text.color);
            }

            // text - main
            qtransform_t ttm = {0};
            ttm.p = (pos2_t){
                tm.p.x + cfg->main.frame.thickness + cfg->main.padding.left,
                tm.p.y + cfg->main.frame.thickness + cfg->main.padding.top + draw_line_idx * (cfg->text.line_spacing + font_size_y)
            };
            dze_qrenderer_draw_text(cfg->text.font, ttm.p, ttm.r, buf, cfg->text.color);

            line_num++;
            draw_line_idx++;
            if (draw_line_idx >= draw_line_num_max)
                break;

            // TODO: IMPROVE: line number of last trailing newline is not shown
        }

        // TODO: IMPL: scrolling
    }

    // ##################################################################
    // function definitions (static)
    // ##################################################################
    
    static void cfg_panel_init_default(cfg_panel_t* cfg) {
        *cfg = {0};

        cfg->main.bg.color = COLOR_U8(21, 25, 35);
        cfg->main.frame.color = COLOR_U8(180, 180, 180);
        cfg->main.frame.thickness = 1;
        cfg->main.padding = (rect_offset_t){ 10.0f, 5.0f, 5.0f, 5.0f };

        cfg->left.bg.color = COLOR_U8(6, 10, 20);
        cfg->left.padding = (rect_offset_t){ 10.0f, 5.0f, 5.0f, 5.0f };

        cfg->text.font = &s_font_default;
        cfg->text.color = COLOR_WHITE;
        cfg->text.line_spacing = 5.0f;
        cfg->text.tab_space_count = 4;

        cfg->left_is_enabled = true;
    }

    static bool get_next_draw_line(doc::iter_t* it, doc::slice_t* slice, const char** slice_progress, char* buf, u64 buf_len, const cfg_panel_t* cfg) {
        // TODO: IMPL: handle cutting lines being to long to be fully drawn

        const char* pread;
        const char* pread_end;
        char* pwrite = buf;
        char* pwrite_end = buf + buf_len; // TODO: IMPL: OOB

        do {
            if (*slice_progress == NULL) {
                if (!doc::next(it, slice)) {
                    if (pwrite == buf) {
                        return false;
                    }
                    goto ret; // yay !! goto !!
                }
                *slice_progress = slice->buf;
            }

            pread = *slice_progress;
            pread_end = slice->buf + slice->len;

            while (pread != pread_end) {
                char c = *pread++;
                switch (c) {
                    case '\n': goto ret; // yay !! goto !!
                    case '\r': break;
                    case '\t':
                        for (u64 i=0; i<cfg->text.tab_space_count; i++)
                            *pwrite++ = ' ';
                        break;
                    default: *pwrite++ = c; break;
                }
            }

            *slice_progress = NULL;
        } while (true);

    ret:
        *slice_progress = pread;
        *pwrite = '\0';
        return true;
    }

}
