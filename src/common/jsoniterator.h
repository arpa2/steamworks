/*
Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * A "multi-iterator" across zero or more attributes
 * of a JSON object, which produces tuples for each
 * combination of the attributes-values.
 *
 * Taking each attribute $a$ as a set of attribute-values
 * $A_a$, produces the cartesian product $A_{a1} \cross A_{a2} .. $
 * as a list.
 */
#ifndef STEAMWORKS_COMMON_JSONITERATOR_H
#define STEAMWORKS_COMMON_JSONITERATOR_H

#include <picojson.h>
#include "logger.h"

class MultiIterator
{
public:
	using inner_value_t = std::string;
	using value_t = std::vector<inner_value_t>;

private:
	using inner_list_t = std::forward_list<inner_value_t>;
	using inner_iterator = inner_list_t::const_iterator;
	using outer_list_t = std::vector<inner_list_t>;
	using outer_iterator = outer_list_t::const_iterator;

	std::vector<bool> m_listy;  // Is the thing in each position list-ish, or a constanct value?
	value_t m_constants;
	std::vector<picojson::array::const_iterator> m_begins, m_ends, m_iter;
	size_t m_size;

public:
	MultiIterator(const picojson::object& object, const std::vector<std::string>& names) :
		m_listy(names.size()),
		m_constants(names.size()),
		m_begins(names.size()),
		m_ends(names.size()),
		m_iter(names.size()),
		m_size(names.size())
	{
		// auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

		unsigned int nameindex = 0;
		for (const auto& f : names)
		{
			std::string object_name;  // Attribute name (case-insensitive) in object
			// log.debugStream() << "MultiIterator using dimension " << nameindex << ' ' << f;

			// Look up (case-insensitive) the name actually used in the object
			for (const auto& attrname : object)
			{
				if (strcasecmp(f.c_str(), attrname.first.c_str()) == 0)
				{
					object_name = attrname.first;
				}
			}

			// log.debugStream() << "  .. attr " << object_name;
			if (object_name.empty())
			{
				// Not mapped
				m_listy[nameindex] = false;
				m_constants[nameindex] = std::string();
				continue;
			}

			const picojson::value& v = object.at(object_name);
			// log.debugStream() << "  .. val " << v.to_str();

			if (v.is<picojson::array>())
			{
				const picojson::array& vv = v.get<picojson::array>();
				m_listy[nameindex] = true;
				// Skip m_constantsp[nameindex]
				m_begins[nameindex] = vv.begin();
				m_iter[nameindex] = vv.begin();
				m_ends[nameindex] = vv.end();
			}
			else
			{
				m_listy[nameindex] = false;
				m_constants[nameindex] = v.get<std::string>();
			}
			nameindex++;
		}
	}

	bool is_done() const
	{
		for (unsigned int i = 0; i<m_size; i++)
		{
			if (m_listy[i] && (m_iter[i] != m_ends[i]))
			{
				return false;
			}
		}
		return true;
	}

	value_t next()
	{
		if (is_done())
		{
			return value_t();
		}

		value_t v;
		for (unsigned int i = 0; i<m_size; i++)
		{
			if (m_listy[i])
			{
				if (m_iter[i] == m_ends[i])
				{
					m_iter[i] = m_begins[i];
				}
				v.push_back((*m_iter[i]).get<std::string>());
			}
			else
			{
				v.push_back(m_constants[i]);
			}
		}

		for (unsigned int i = 0; i<m_size; i++)
		{
			if (m_listy[i] && (++m_iter[i] != m_ends[i]))
			{
				break;
			}
		}
		return v;
	}
} ;

#endif
