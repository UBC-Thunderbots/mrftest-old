#ifndef UTIL_XML_H
#define UTIL_XML_H

#include <libxml++/libxml++.h>
#include <glibmm/ustring.h>

namespace xmlutil {
	//
	// Finds the only child of "parent" whose name is "name" and returns it. If
	// no children of "parent" are named "name", or if there are multiple
	// matching children, delete all existing children of that name; create a
	// new, empty child of the given name; and return the new child.
	//
	xmlpp::Element *get_only_child(xmlpp::Element *parent, const Glib::ustring &name);

	//
	// Finds the only child of "parent" whose name is "name" and who has an
	// attribute "attrname" whose value is "attrval". If there are multiple
	// such children, deletes them all. If none found, creates a new, empty
	// element, adds attribute "attrname" with value "attrval", and returns the
	// new element.
	//
	xmlpp::Element *get_only_child_keyed(xmlpp::Element *parent, const Glib::ustring &name, const Glib::ustring &attrname, const Glib::ustring &attrval);

	//
	// Removes all content nodes and entity references that are children of the
	// given element. This leaves just elements and attributes behind.
	//
	xmlpp::Element *strip(xmlpp::Element *parent);
}

#endif

