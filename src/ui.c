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

    typedef enum {
        TEXT_COLOR_MAIN,
        TEXT_COLOR_LEFT,
        TEXT_COLOR_COUNT,
    } color_text_t;

    typedef struct {
        font_t* font;
        color_t color[TEXT_COLOR_COUNT];

        float line_spacing;
        u64 tab_space_count;
    } cfg_text_t;

    typedef struct {
        cfg_section_t main;
        cfg_section_t left;
        cfg_text_t text;

        bool show_left;
        bool soft_wrap;
    } cfg_panel_t;

    typedef struct {
        float size_y;
        float size_x_num;
    } font_info_t;

    typedef struct {
        doc::slice_t slice;
        u64 pos;
    } drawline_select_t;

    typedef struct {
        u64 line_num;
        char buf[1024];
        bool has_newline;
    } drawline_t;

    typedef enum {
        ALIGN_BEGIN,
        ALIGN_CENTER,
        ALIGN_END,
    } align_t;

    // ##################################################################
    // function declarations (static)
    // ##################################################################

    static void cfg_panel_init_default(cfg_panel_t*);

    static bool drawline_next(doc::iter_t*, drawline_select_t*, drawline_t* retv, float drawline_size_x_max, const cfg_panel_t*);
    static bool get_char(doc::iter_t*, drawline_select_t*, char* retv);
    
    static size2_t cfg_section_border_size(const cfg_section_t*);
    static size2_t cfg_section_content_area_size(const cfg_section_t*, size2_t);

    static font_info_t calc_font_info(font_t* font);
    static u64 calc_digits(u64 num);

    static void draw_text(font_t*, qtransform_t, float offset_y, const cfg_section_t*, align_t align, const char* text, color_t);

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

        font_info_t finfo = calc_font_info(cfg->text.font);
        float line_offset = finfo.size_y + cfg->text.line_spacing;

        u64 drawline_max = cfg_section_content_area_size(&cfg->main, size).y /*only y can be used up here*/ / line_offset;
        u64 line_num_digits_max = calc_digits(drawline_max);
        
        // area - left
        qtransform_t t_la = {0};
        if (cfg->show_left) {
            t_la.p = pos;
            t_la.s = (size2_t){ cfg_section_border_size(&cfg->left).x + line_num_digits_max * finfo.size_x_num, size.y };
            dze_qrenderer_draw_quad(t_la, cfg->left.bg.color);
            dze_qrenderer_draw_frame(t_la, cfg->left.frame.thickness, cfg->left.frame.color);
        }

        // area - main
        qtransform_t t_ma = {0};
        t_ma.p = (pos2_t){ t_la.p.x + t_la.s.x, pos.y };
        t_ma.s = (size2_t){ size.x - t_la.s.x, size.y };
        dze_qrenderer_draw_quad(t_ma, cfg->main.bg.color);
        dze_qrenderer_draw_frame(t_ma, cfg->main.frame.thickness, cfg->main.frame.color);

        //
        size2_t main_content_area_size = cfg_section_content_area_size(&cfg->main, t_ma.s);

        drawline_select_t select = {0};
        drawline_t drawline = {0}; drawline.has_newline = true;
        u64 drawline_idx = 0;
        do {
            float line_offset_final = drawline_idx * line_offset;

            // text - left
            if (drawline.has_newline) {
                if (cfg->show_left) {
                    char line_num_buf[21];
                    sprintf(line_num_buf, "%lu", drawline.line_num + 1);
                    draw_text(cfg->text.font, t_la, line_offset_final, &cfg->left, ALIGN_END, line_num_buf, cfg->text.color[TEXT_COLOR_LEFT]);
                }
            }

            //
            if (!drawline_next(it, &select, &drawline, main_content_area_size.x, cfg))
                break;

            // text - main
            draw_text(cfg->text.font, t_ma, line_offset_final, &cfg->main, ALIGN_BEGIN, drawline.buf, cfg->text.color[TEXT_COLOR_MAIN]);

            //
            drawline_idx++;
            if (drawline_idx >= drawline_max)
                break;
        } while(true);

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
        cfg->text.color[TEXT_COLOR_MAIN] = COLOR_WHITE;
        cfg->text.color[TEXT_COLOR_LEFT] = COLOR_U8(150, 150, 150);
        cfg->text.line_spacing = 5.0f;
        cfg->text.tab_space_count = 4;

        cfg->show_left = true;
        cfg->soft_wrap = true;
    }

    static bool drawline_next(doc::iter_t* it, drawline_select_t* select, drawline_t* retv, float drawline_size_x_max, const cfg_panel_t* cfg) {
        char* pbuf = retv->buf;
        // TODO: IMPL: OOB for pbuf

        retv->has_newline = false;

        char c;
        while (get_char(it, select, &c)) {
            char* pbuf_backup = pbuf;

            switch (c) {
                case '\n':
                    retv->line_num++;
                    retv->has_newline = true;
                    goto ret; // yay !! goto !!
                case '\r': break;
                case '\t':
                    for (u64 i=0; i<cfg->text.tab_space_count; i++)
                        *pbuf++ = ' ';
                    break;
                default:
                    *pbuf++ = c;
                    break;
            }

            // TODO: IMPROVE: do not calculate everything for each char
            *pbuf = '\0';
            size2_t size = dze_font_sizeof(cfg->text.font, retv->buf);
            if (size.x > drawline_size_x_max) {
                pbuf = pbuf_backup;
                select->pos--;

                if (cfg->soft_wrap)
                    goto ret; // yay !! goto !!

                while (get_char(it, select, &c)) {
                    if (c == '\n') {
                        retv->line_num++;
                        goto ret; // yay !! goto !!
                    }
                }
            }
        }

        return pbuf != retv->buf;
    ret:
        *pbuf = '\0';
        return true;
    }

    static bool get_char(doc::iter_t* it, drawline_select_t* select, char* retv) {
        // TODO: IMPROVE: fetching each character individually is not the way to go. but its fine for now
        if (select->pos >= select->slice.len) {
            if (!doc::next(it, &select->slice))
                return false;
            select->pos = 0;
            // expect new slice to at least contain 1 character
        }
        *retv = select->slice.buf[select->pos++];
        return true;
    }

    static font_info_t calc_font_info(font_t* font) {
        static const char* FONT_CHARACTERS_VISIBLE = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        static const char* FONT_CHARACTERS_NUM = "0123456789";

        font_info_t info;

        info.size_y = dze_font_sizeof(font, FONT_CHARACTERS_VISIBLE).y;
        info.size_x_num = 1.0f;
        char buf_num[2]; buf_num[1] = '\0';
        for (const char* c = FONT_CHARACTERS_NUM; *c != '\0'; c++) {
            buf_num[0] = *c;
            float size_x_num = dze_font_sizeof(font, buf_num).x;
            if (size_x_num > info.size_x_num)
                info.size_x_num = size_x_num;
        }

        return info;
    }

    static u64 calc_digits(u64 num) {
        u64 digits = 1;
        if (num >= 100000000) { digits += 8; num /= 100000000; }
        if (num >= 10000) { digits += 4; num /= 10000; }
        if (num >= 100) { digits += 2; num /= 100; }
        if (num >= 10) { digits += 1; }
        return digits;
    }

    static size2_t cfg_section_border_size(const cfg_section_t* cfg) {
        return (size2_t){
            2 * cfg->frame.thickness + cfg->padding.left + cfg->padding.right,
            2 * cfg->frame.thickness + cfg->padding.top + cfg->padding.bottom,
        };
    }

    static size2_t cfg_section_content_area_size(const cfg_section_t* cfg, size2_t size) {
        size2_t border = cfg_section_border_size(cfg);
        return glms_vec2_sub(size, border);
    }

    static void draw_text(font_t* font, qtransform_t area, float offset_y, const cfg_section_t* section, align_t align, const char* text, color_t color) {
        size2_t text_size;
        if (align != ALIGN_BEGIN)
            text_size = dze_font_sizeof(font, text);

        qtransform_t t = {0};
        switch (align) {
            case ALIGN_BEGIN:
                t.p.x = area.p.x + section->frame.thickness + section->padding.left;
                break;
            case ALIGN_CENTER:
                t.p.x = area.p.x + (area.s.x - text_size.x) / 2.0f;
                break;
            case ALIGN_END:
                t.p.x = area.p.x + area.s.x - section->frame.thickness - section->padding.right - text_size.x;
                break;
        }
        t.p.y = area.p.y + section->frame.thickness + section->padding.top + offset_y;
        
        dze_qrenderer_draw_text(font, t.p, t.r, text, color);
    }

}
