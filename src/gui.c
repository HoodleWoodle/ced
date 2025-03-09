#include "gui.h"
#include "common.h"

#include <zwerg.h>

namespace ced::gui {
    static const char* FONT_VISIBLE_CHARACTERS = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

    static const char* FONT_FILE = "fonts/Share_Tech_Mono/ShareTechMono-Regular.ttf";
    static const u64   FONT_SIZE = 28;

    static const color_t COLOR_BG = { 0.25f, 0.30f, 0.25f, 1.0f };
    static const color_t COLOR_FG = COLOR_WHITE;
    
    static const float TEXT_MARGIN_X   = 5.0f;
    static const float TEXT_MARGIN_Y   = 5.0f;
    static const float TEXT_PADDING_Y  = 5.0f;
    static const u64   TEXT_TAB_SPACES = 4;

    static font_t s_font;
    static float  s_font_height;
    static u64    s_draw_line_idx;

    void init() {
        dze_window_create("ced", 1280, 720);
        dze_qrenderer_init(10000);

	    dze_font_init(&s_font, FONT_FILE, FONT_SIZE);

	    glClearColor(COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a); GL_CHECK();
    }

    void deinit() {
        dze_font_free(&s_font);
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

        // TODO: IMPROVE: kinda awkward to do it here - but fonts in zwerg need to be live adjusted
        s_font_height = dze_font_sizeof(&s_font, FONT_VISIBLE_CHARACTERS).y;

        s_draw_line_idx = 0;
    }

    void frame_end() {
		dze_qrenderer_end();
		dze_window_update();
    }

    static void text_draw(const char* text) {
        qtransform_t t;
        t.s = dze_font_sizeof(&s_font, text);
        t.p = (pos2_t){TEXT_MARGIN_X, TEXT_MARGIN_Y + s_draw_line_idx * (TEXT_PADDING_Y + s_font_height) };
        t.r = (rotation_t)glm_rad(0.0f);

        dze_qrenderer_draw_text(&s_font, t.p, t.r, text, COLOR_FG);
    }

    void text(const char* text) {
        const char* ptr_read = text;

        char line_buf[255];
        char* ptr_write = line_buf;

        const char* begin = text;
        for (char c; (c = *ptr_read) != '\0'; ptr_read++) {
            switch (c) {
                case '\n':
                    *ptr_write = '\0';
                    text_draw(line_buf);
                    ptr_write = line_buf;
                    s_draw_line_idx++;
                    break;
                case '\t':
                    for (int i=0; i<TEXT_TAB_SPACES; i++) {
                        *ptr_write = ' '; // TODO: FIX: OOB
                        ptr_write++;
                    }
                    // TODO: IMPL
                    break;
                default:
                    *ptr_write = c; // TODO: FIX: OOB
                    ptr_write++;
                    break;
            }
        }

        *ptr_write = '\0';
        text_draw(line_buf);
    }
}