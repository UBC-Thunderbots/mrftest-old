#include "world/config.h"
#include "util/xml.h"
#include <algorithm>

xmlpp::Element *xmlutil::get_only_child(xmlpp::Element *parent, const Glib::ustring &name) {
	// First get a list of all matching children.
	const xmlpp::Node::NodeList &matching = parent->get_children(name);

	// Go through the list, counting how many of these children are elements. At the same
	// time, delete any that aren't elements.
	unsigned int matching_elements = 0;
	xmlpp::Element *found_element = 0;
	for (xmlpp::Node::NodeList::const_iterator i = matching.begin(), iend = matching.end(); i != iend; ++i) {
		xmlpp::Node *node = *i;
		xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
		if (elem) {
			matching_elements++;
			found_element = elem;
		} else {
			parent->remove_child(node);
			config::dirty();
		}
	}

	// If there are multiple matching elements, remove them all because we don't know which
	// one is correct.
	if (matching_elements > 1) {
		const xmlpp::Node::NodeList &matching2 = parent->get_children(name);
		std::for_each(matching2.begin(), matching2.end(), std::bind1st(std::mem_fun(&xmlpp::Node::remove_child), parent));
		found_element = 0;
		config::dirty();
	}

	// If at this point we do not have an element to return, create a new one.
	if (!found_element) {
		found_element = parent->add_child(name);
		config::dirty();
	}

	// Done!
	return found_element;
}

xmlpp::Element *xmlutil::get_only_child_keyed(xmlpp::Element *parent, const Glib::ustring &name, const Glib::ustring &attrname, const Glib::ustring &attrval) {
	// First get a list of all matching children.
	const xmlpp::Node::NodeList &matching = parent->get_children(name);

	// Go through the list, counting how many of these children are elements with the proper attribute value.
	// At the same time, delete any that aren't elements.
	unsigned int matching_elements = 0;
	xmlpp::Element *found_element = 0;
	for (xmlpp::Node::NodeList::const_iterator i = matching.begin(), iend = matching.end(); i != iend; ++i) {
		xmlpp::Node *node = *i;
		xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
		if (elem) {
			if (elem->get_attribute_value(attrname) == attrval) {
				matching_elements++;
				found_element = elem;
			}
		} else {
			parent->remove_child(node);
			config::dirty();
		}
	}

	// If there are multiple matching elements, remove them all because we don't know which
	// one is correct.
	if (matching_elements > 1) {
		const xmlpp::Node::NodeList &matching2 = parent->get_children(name);
		for (xmlpp::Node::NodeList::const_iterator i = matching2.begin(), iend = matching2.end(); i != iend; ++i) {
			xmlpp::Node *node = *i;
			xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
			if (elem && elem->get_attribute_value(attrname) == attrval)
				parent->remove_child(elem);
		}
		found_element = 0;
		config::dirty();
	}

	// If at this point we do not have an element to return, create a new one.
	if (!found_element) {
		found_element = parent->add_child(name);
		found_element->set_attribute(attrname, attrval);
		config::dirty();
	}

	// Done!
	return found_element;
}

xmlpp::Element *xmlutil::strip(xmlpp::Element *parent) {
	// Get a list of all children.
	const xmlpp::Node::NodeList &children = parent->get_children();

	// Iterate the list.
	for (xmlpp::Node::NodeList::const_iterator i = children.begin(), iend = children.end(); i != iend; ++i) {
		xmlpp::Node *node = *i;

		// Check if it's a content node or entity reference.
		if (dynamic_cast<xmlpp::ContentNode *>(node) || dynamic_cast<xmlpp::EntityReference *>(node)) {
			// Remove it.
			parent->remove_child(node);
		}
	}

	// Done.
	return parent;
}

