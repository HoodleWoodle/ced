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
    } margin_t;

    typedef struct {
        color_t color;
    } cfg_bg_t;

    typedef struct {
        float thickness;
        color_t color;
    } cfg_frame_t;

    typedef struct {
        font_t* font;
        color_t color;
        margin_t margin;
        float padding;

        u64 tab_space_count;
    } cfg_text_t;

    typedef struct {
        cfg_bg_t bg;
        cfg_frame_t frame;
        cfg_text_t text;
    } cfg_panel_t;

    // ##################################################################
    // function declarations (static)
    // ##################################################################

    static void cfg_panel_init_default(cfg_panel_t* cfg);
    
    static bool get_next_draw_line(doc::iter_t* it, doc::slice_t* slice, const char** slice_progress, char* buf, u64 buf_len, const cfg_panel_t* cfg);

    // ##################################################################
    // constants
    // ##################################################################

    static const char* FONT_VISIBLE_CHARACTERS = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

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

        qtransform_t t = {0}; t.p = pos; t.s = size;
        dze_qrenderer_draw_quad(t, cfg->bg.color);
        dze_qrenderer_draw_frame(t, cfg->frame.thickness, cfg->frame.color);

        float font_size_y = dze_font_sizeof(cfg->text.font, FONT_VISIBLE_CHARACTERS).y;
        u64 draw_line_max = (t.s.y - 2 * cfg->frame.thickness) / (font_size_y + cfg->text.padding);
        u64 draw_line_idx = 0;

        doc::slice_t slice = {0};
        const char* slice_progress = NULL;

        char buf[1024];
        while (get_next_draw_line(it, &slice, &slice_progress, buf, sizeof(buf), cfg)) {
            qtransform_t tt = {0};
            tt.p = (pos2_t){
                t.p.x + cfg->frame.thickness + cfg->text.margin.left,
                t.p.y + cfg->frame.thickness + cfg->text.margin.top + draw_line_idx * (cfg->text.padding + font_size_y)
            };
            dze_qrenderer_draw_text(cfg->text.font, tt.p, tt.r, buf, cfg->text.color);

            draw_line_idx++;
            if (draw_line_idx >= draw_line_max)
                break;
        }

        // TODO: IMPL: scrolling
    }

    // ##################################################################
    // function definitions (static)
    // ##################################################################
    
    static void cfg_panel_init_default(cfg_panel_t* cfg) {
        cfg->bg.color = COLOR_U8(21, 25, 35);

        cfg->text.font = &s_font_default;
        cfg->text.color = COLOR_WHITE;
        cfg->text.margin = (margin_t){ 5.0f, 5.0f, 5.0f, 5.0f };
        cfg->text.padding = 5.0f;
        cfg->text.tab_space_count = 4;

        //COLOR_U8(6, 10, 20)
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
                    break;
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
