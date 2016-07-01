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

#include "picojson.h"

class MultiIterator
{
    using inner_value_t = std::string;
    using inner_list_t = std::forward_list<inner_value_t>;
    using inner_iterator = inner_list_t::const_iterator;
    using outer_list_t = std::vector<inner_list_t>;
    using outer_iterator = outer_list_t::const_iterator;
    using value_t = std::vector<inner_value_t>;

private:
    std::vector<inner_iterator> m_begins, m_ends, m_iter;

public:
    MultiIterator(const outer_list_t& list_of_lists_of_values)
    {
        for (const auto& f : list_of_lists_of_values)
        {
            auto it = f.cbegin();
            m_begins.push_back(it);
            m_iter.push_back(it);
            m_ends.push_back(f.cend());
        }
    }

    bool is_done() const
    {
        for (unsigned int i = 0; i<m_begins.size(); i++)
        {
            if (m_iter[i] != m_ends[i])
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
        for (unsigned int i = 0; i<m_begins.size(); i++)
        {
            if (m_iter[i] == m_ends[i])
            {
                m_iter[i] = m_begins[i];
            }
            v.push_back(*m_iter[i]);
        }

        unsigned int count_wrapped;
        for (unsigned int i = 0; i<m_begins.size(); i++)
        {
            if (++m_iter[i] != m_ends[i])
            {
                break;
            }
            count_wrapped++;
        }
        return v;
    }
} ;

#endif
