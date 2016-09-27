//
// open horizon -- undefined_darkness@outlook.com
//

#include "util/util.h"

#include "containers/cdp.h"
#include "containers/pac5.h"
#include "containers/pac6.h"
#include "containers/poc.h"
#include "formats/tga.h"
#include "renderer/texture_gim.h"
#include "zip.h"

//------------------------------------------------------------

inline std::string tex_name(std::string base, int idx) { return base + (idx < 10 ? "0" : "" ) + std::to_string(idx) + ".tga"; }
inline std::string loc_name(std::string base, int idx) { return base + (idx < 10 ? "0" : "" ) + std::to_string(idx); }
inline std::string loc_filename(std::string base, int idx) { return base + "_loc" + (idx < 10 ? "0" : "" ) + std::to_string(idx) + ".zip"; }

inline nya_memory::tmp_buffer_ref load_resource(poc_file &p, int idx)
{
    nya_memory::tmp_buffer_ref b(p.get_chunk_size(idx));
    if (!p.read_chunk_data(idx, b.get_data()))
        b.free();

    return b;
}

inline int clamp_int(int val, int to) { return val < 0 ? 0 : (val < to ? val : to - 1); }

//------------------------------------------------------------

bool convert_location5(const void *data, size_t size, std::string name, std::string filename)
{
    poc_file p;
    if (!p.open(data, size))
        return false;

    zip_t *zip = zip_open(filename.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 0);
    if (!zip)
    {
        printf("Unable to save location %s\n", filename.c_str());
        return false;
    }

    std::string info_str = "<!--Open Horizon location-->\n";
    info_str += "<location name=\"" + name + "\">\n";

    nya_memory::tmp_buffer_scoped res(load_resource(p, 7));
    nya_memory::tmp_buffer_scoped pal_res(load_resource(p, 8));

    struct tex_header
    {
        uint32_t count;
        uint32_t mip_offsets[4];
        uint32_t padding[3];
    };

    if (res.get_size() >= sizeof(tex_header))
    {
        //params
        const int tex_size = 1024;
        const int frag_size = 64;
        const int bord_size = 2;

        const auto header = (tex_header *)res.get_data();

        union color
        {
            unsigned int u;
            struct { unsigned char b, g, r, a; };
        };

        assert(pal_res.get_size() == 256 * 4);
        color *pal = (color *)pal_res.get_data();
        color palette[256];
        for (int i = 0; i < 256; i+=32)
        {
            memcpy(&palette[i], &pal[i], 32);
            memcpy(&palette[i + 16], &pal[i + 8], 32);
            memcpy(&palette[i + 8], &pal[i + 16], 32);
            memcpy(&palette[i + 24], &pal[i + 24], 32);
        }

        for(auto &p: palette)
        {
            std::swap(p.r, p.b);
            p.a = p.a > 127 ? 255 : p.a * 2;
        }

        struct frag { unsigned char data[frag_size][frag_size]; };

        assert(res.get_size() >= header->mip_offsets[0] + sizeof(frag) * header->count);

        const int frag_per_line = tex_size / (frag_size + bord_size * 2);
        const int frag_per_tex = frag_per_line * frag_per_line;

        nya_formats::tga tga;
        tga.width = tga.height = tex_size;
        tga.channels = nya_formats::tga::bgra;

        const frag *f = (frag *)res.get_data(header->mip_offsets[0]), *last_f = f + header->count;
        const int tiles_count = header->count / frag_per_tex + 1;
        for (int i = 0; i < tiles_count; ++i)
        {
            nya_memory::tmp_buffer_scoped buf(tex_size * tex_size * tga.channels + nya_formats::tga::tga_minimum_header_size);
            memset(buf.get_data(), 0, buf.get_size());

            tga.encode_header(buf.get_data());

            color *tdata = (color *)buf.get_data(nya_formats::tga::tga_minimum_header_size);
            for (int y = 0; y < frag_per_line && f < last_f; ++y)
            {
                color *lv = tdata + tex_size * ((frag_size + bord_size * 2) * y + bord_size);
                for (int x = 0; x < frag_per_line && f < last_f; ++x, ++f)
                {
                    color *lh = lv + x * (frag_size + bord_size * 2);
                    for (int dy = -bord_size; dy < frag_size + bord_size; ++dy)
                    {
                        color *ldv = lh + tex_size * dy + bord_size;
                        for (int dx = -bord_size; dx < frag_size + bord_size; ++dx)
                            ldv[dx] = palette[f->data[clamp_int(dx, frag_size)][clamp_int(dy, frag_size)]];
                    }
                }
            }

            zip_entry_open(zip, tex_name("land", i).c_str());
            zip_entry_write(zip, buf.get_data(), buf.get_size());
            zip_entry_close(zip);
        }

        info_str += "\t<tiles count=\"" + std::to_string(tiles_count) + "\" " +
                    "size=\"" + std::to_string(frag_size) + "\" " +
                    "border=\"" + std::to_string(bord_size) + "\"/>\n";
    }

    info_str += "</location>\n\n";

    zip_entry_open(zip, "info.xml");
    zip_entry_write(zip, info_str.c_str(), info_str.size());
    zip_entry_close(zip);

    zip_close(zip);

    return true;
}

//------------------------------------------------------------

int main(int argc, const char * argv[])
{
    std::string src_path;
    std::string dst_path = "locations/";

    if (argc > 1)
    {
        src_path = argv[1];
        if (!src_path.empty() && src_path.back() != '/')
            src_path.push_back('/');
    }

    if (argc > 2)
    {
        dst_path = argv[2];
        if (!dst_path.empty() && dst_path.back() != '/')
            dst_path.push_back('/');
    }

    cdp_file pak4;
    pac5_file pak5;
    pac6_file pak6;

    if (pak4.open((src_path + "DATA.CDP").c_str()))
    {
        //ToDo: AC4
    }
    else if (pak5.open((src_path + "DATA.PAC").c_str()))
    {
        if (pak5.get_files_count() < 1000) //ac5
        {
            for (int i = 3, idx = 0; i <= 72; ++i, ++idx)
            {
                nya_memory::tmp_buffer_scoped b(pak5.get_file_size(i));
                if (!pak5.read_file_data(i, b.get_data()))
                    continue;

                convert_location5(b.get_data(), b.get_size(), loc_name("AC5", idx), loc_filename(dst_path + "ac5", idx));
            }
        }
        else
        {
            //ToDo: ACZ
        }
    }
    else if (pak6.open((src_path + "DATA00.PAC").c_str()))
    {
        //ToDo: AC6
    }
    else
        printf("No data found in src directory.\n");

    return 0;
}

//------------------------------------------------------------
