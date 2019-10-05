#include "util/config.h"
#include <glibmm/convert.h>
#include <glibmm/ustring.h>
#include <libgen.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <vector>
#include "util/exception.h"

namespace
{
/**
 * \return The parser.
 */
xmlpp::DomParser &parser()
{
    static xmlpp::DomParser instance;
    return instance;
}

/**
 * \return the directory in which the running executable is stored.
 */
std::string get_bin_directory()
{
    std::vector<char> buffer(64);
    ssize_t retval;
    while ((retval = readlink("/proc/self/exe", &buffer[0], buffer.size())) ==
           static_cast<ssize_t>(buffer.size()))
    {
        buffer.resize(buffer.size() * 2);
    }
    if (retval < 0)
    {
        throw SystemError("readlink(/proc/self/exe)", errno);
    }
    if (retval < static_cast<ssize_t>(buffer.size()))
    {
        buffer[static_cast<std::size_t>(retval)] = '\0';
    }
    else
    {
        buffer.push_back('\0');
    }
    return dirname(&buffer[0]);
}

/**
 * \return the filename of the config file.
 */
std::string get_config_filename()
{
    return get_bin_directory() + "/../config.xml";
}

/**
 * \brief Removes all text nodes that contain only whitespace.
 *
 * \param[in] elt the element from which text nodes should be clean.
 */
void clean_whitespace(xmlpp::Element *e)
{
    std::vector<xmlpp::Node *> to_remove;
    for (xmlpp::Node *i : e->get_children())
    {
        xmlpp::TextNode *t = dynamic_cast<xmlpp::TextNode *>(i);
        if (t)
        {
            const Glib::ustring &text = t->get_content();
            bool found                = false;
            for (auto j = text.begin(), jend = text.end(); !found && j != jend;
                 ++j)
            {
                if (!(*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r'))
                {
                    found = true;
                }
            }
            if (!found)
            {
                to_remove.push_back(t);
            }
        }
    }
    std::for_each(to_remove.begin(), to_remove.end(), [e](xmlpp::Node *n) {
        e->remove_child(n);
    });
}
}

void Config::load()
{
    std::ifstream ifs;
    ifs.exceptions(
        std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(
        get_config_filename().c_str(),
        std::ifstream::in | std::ifstream::binary);
    ifs.exceptions(std::ifstream::goodbit);
    parser().set_validate();
    parser().set_substitute_entities();
    parser().parse_stream(ifs);
    const xmlpp::Document *document = parser().get_document();
    xmlpp::Element *root_elt        = document->get_root_node();
    if (root_elt->get_name() != u8"thunderbots")
    {
        throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(
            u8"Malformed config.xml (expected root element of type "
            u8"thunderbots, found %1)",
            root_elt->get_name())));
    }
    clean_whitespace(root_elt);
}

void Config::save()
{
    xmlpp::Document *doc = parser().get_document();
    std::ofstream ofs;
    ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(
        get_config_filename().c_str(),
        std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    doc->write_to_stream_formatted(ofs);
}

xmlpp::Element *Config::params()
{
    xmlpp::Document *doc     = parser().get_document();
    xmlpp::Element *root_elt = doc->get_root_node();
    const xmlpp::Node::NodeList &params_elts =
        root_elt->get_children(u8"params");
    if (params_elts.size() != 1)
    {
        throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(
            u8"Malformed config.xml (expected exactly one params element, "
            u8"found %1)",
            params_elts.size())));
    }
    return dynamic_cast<xmlpp::Element *>(*params_elts.begin());
}

xmlpp::Element *Config::joysticks()
{
    xmlpp::Document *doc     = parser().get_document();
    xmlpp::Element *root_elt = doc->get_root_node();
    const xmlpp::Node::NodeList &joysticks_elts =
        root_elt->get_children(u8"joysticks");
    if (joysticks_elts.size() != 1)
    {
        throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(
            u8"Malformed config.xml (expected exactly one joysticks element, "
            u8"found %1)",
            joysticks_elts.size())));
    }
    return dynamic_cast<xmlpp::Element *>(*joysticks_elts.begin());
}
