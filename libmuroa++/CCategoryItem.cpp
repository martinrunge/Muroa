/***************************************************************************
 *
 *   CCategoryItem.cpp
 *
 *   This file is part of libmuroa++                                  *
 *   Copyright (C) 2011 by Martin Runge <martin.runge@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "CRootItem.h"
#include "CCategoryItem.h"
#include "IContentItem.h"
#include "CDiff.h"

#include <iostream>
#include <cassert>
#include <algorithm>

using namespace std;
namespace muroa {

CCategoryItem::CCategoryItem(CRootItem *root_item, string text, CCategoryItem*  parent) : CItemBase(root_item, parent, CItemType::E_CAT)
{
	replaceTabs(text);
	m_name = text;

	if(m_parent) {
		m_path = m_parent->getPath();
	}
	// if m_path does not end with a slash, append one
	size_t lastslashpos = m_path.rfind('/');
	if( m_path.size() == 0 || lastslashpos != m_path.size() - 1 ) {
		m_path.append("/");
	}
	m_path.append(m_name);
	if(m_parent) {
		m_parent->addChild(this);
	}
	m_root_item->setCategoryPtr(m_path, this);
}

CCategoryItem::CCategoryItem(const CCategoryItem& other, CRootItem *root_item, CCategoryItem* parent) : CItemBase(root_item, parent, CItemType::E_CAT)
{
    m_path = other.m_path;
    m_name = other.m_name;

    for(int i = 0; i < other.m_sub_categories.size(); i++) {
        m_sub_categories.push_back(new CCategoryItem(*(other.m_sub_categories[i]), root_item, this ));
    }


    for(int i = 0; i < other.m_content_items.size(); i++) {
        IContentItem* cItem = IContentItem::itemFactory(*(other.m_content_items[i]), root_item, this);
        m_content_items.push_back(cItem);
    }

    root_item->setCategoryPtr(m_path, this);
}

CCategoryItem::~CCategoryItem() {
	std::vector<CCategoryItem*>::iterator cit;
	for(cit = m_sub_categories.begin(); cit != m_sub_categories.end(); cit++ ) {
		delete *cit;
	}
	m_sub_categories.clear();

	std::vector<IContentItem*>::iterator mit;
	for(mit = m_content_items.begin(); mit != m_content_items.end(); mit++ ) {
		delete *mit;
	}
	m_content_items.clear();
	m_root_item->delCategoryPtr(m_path);
}

void CCategoryItem::addChild(CCategoryItem* newSubCategory) {
	// binary search for insert pos
	vector<CCategoryItem*>::iterator up;
	up = upper_bound(m_sub_categories.begin(), m_sub_categories.end(), newSubCategory, CompareCategoriesByName() );

	unsigned insertPos = up - m_sub_categories.begin();
	m_root_item->beginInsertItems( insertPos, 1, this );
	m_sub_categories.insert( up, newSubCategory);
	m_root_item->endInsertItems();
}

void CCategoryItem::addChild(IContentItem*  newMediaItem, int pos) {
	if(pos == -1) {
		m_root_item->beginInsertItems( m_content_items.size(), 1, this );
		m_content_items.push_back(newMediaItem);
	}
	else {
		m_root_item->beginInsertItems( pos, 1, this );
		m_content_items.insert( m_content_items.begin() + pos - m_sub_categories.size(), newMediaItem );
	}
	m_root_item->endInsertItems();
}

IContentItem* CCategoryItem::getContentItem(unsigned pos) {
	if(m_content_items.size() <= pos ) return 0;
	return m_content_items[pos];
}

CCategoryItem* CCategoryItem::getCategoryItem(std::string name) {
	std::vector<CCategoryItem*>::iterator cit;
	for(cit = m_sub_categories.begin(); cit != m_sub_categories.end(); cit++ ) {
		if(name.compare((*cit)->getName()) == 0 ) {
			return (*cit);
		}
	}
	return 0;
}

/**
 * categories first, media items then
 */
CItemBase* CCategoryItem::childAt(unsigned row) {
	if(row < m_sub_categories.size()) {
		return m_sub_categories[row];
	}
	else {
		row -= m_sub_categories.size();
		if(row < m_content_items.size()) {
			return m_content_items[row];
		}
		else {
			return 0;
		}
	}
}

unsigned CCategoryItem::childPos(const CItemBase* const child) {

	unsigned pos = 0;

	unsigned n_sub_cats = m_sub_categories.size();

	for(pos = 0; pos < n_sub_cats; pos++ ) {
		if( m_sub_categories[pos] == child ) {
			return pos;
		}
	}

	for(pos = n_sub_cats; pos < ( n_sub_cats + m_content_items.size() ); pos++ ) {
		if( m_content_items[pos - n_sub_cats] == child ) {
			return pos;
		}
	}

	return 0;
}


int CCategoryItem::getNumContentItems() {
	return m_content_items.size();
}

int CCategoryItem::getNumCategories() {
	return m_sub_categories.size();
}

int CCategoryItem::numChildren() {
	int num = m_sub_categories.size();
	num += m_content_items.size();
	return num;
}

IContentItem* CCategoryItem::getPredecessorOf(IContentItem* ci) {
	vector<IContentItem*>::iterator it;
	IContentItem* result = 0;
	for( it = m_content_items.begin(); it != m_content_items.end(); it++ )
	{
		if( *it == ci ) {
			return result;
		}
		result = *it;
	}
	return 0;
}

IContentItem* CCategoryItem::getSuccessorOf(IContentItem* ci) {
	vector<IContentItem*>::iterator it;
	for( it = m_content_items.begin(); it != m_content_items.end(); it++ )
	{
		if( *it == ci ) {
			if(++it != m_content_items.end() ) {
				return *it;
			}
			else {
				return 0;
			}
		}
	}
	return 0;
}


void CCategoryItem::delContentItem(int pos) {
	m_root_item->beginRemoveItems(pos, 1, this);
	m_content_items.erase(m_content_items.begin() + pos);
	m_root_item->endRemoveItems();
}

void CCategoryItem::delCategory(CCategoryItem* categoryItem) {
	std::vector<CCategoryItem*>::iterator cit;
	int i = 0;
	for(cit = m_sub_categories.begin(); cit != m_sub_categories.end(); cit++, i++ ) {
		if(categoryItem == *cit) {
			m_root_item->beginRemoveItems(i, 1, this );
			m_sub_categories.erase(cit);
			m_root_item->endRemoveItems();
			break;
		}
	}
}


string CCategoryItem::serialize(bool asDiff) {
	string result;
	// result.append(m_path);
	// result.append("\n");
	std::vector<CCategoryItem*>::iterator cit;
	for(cit = m_sub_categories.begin(); cit != m_sub_categories.end(); cit++ ) {
		result.append((*cit)->serialize(asDiff));
	}

	if(asDiff) {
		ostringstream oss;
		oss << "+++ " << getPath() << endl;
		oss << "@@ -0,0 +1," << m_content_items.size() << endl;
		result.append(oss.str());
	}
	std::vector<IContentItem*>::iterator mit;
	for(mit = m_content_items.begin(); mit != m_content_items.end(); mit++ ) {
		result.append(m_path);
		result.append("\t");
		result.append((*mit)->serialize(asDiff));
	}

	return result;
}

string CCategoryItem::diff(const CCategoryItem* other) {
	string diff;

	vector<CCategoryItem*>::const_iterator cit;
	vector<CCategoryItem*>::const_iterator other_cit = other->m_sub_categories.begin();

	string childpath;
	for(cit = m_sub_categories.begin(); cit != m_sub_categories.end(); cit++ ) {

		childpath = (*cit)->getPath();
		// cerr << "outer for: cit: " << childpath << " other cit: " << (*other_cit)->getPath() << endl;

		if( other_cit == other->m_sub_categories.end() || childpath.compare( (*other_cit)->getPath()) < 0 ) {
			// old is smaller than new or new has no sub_categories left-> this entry disappeared in new
			ostringstream oss;
			oss << "--- " << (*cit)->getPath() << endl;
			diff.append(oss.str());
			continue;
		}

		while( other_cit != other->m_sub_categories.end()) {
			string other_childpath = (*other_cit)->getPath();

			// cerr << "inner while: cit: " << childpath << " other cit: " << other_childpath << " ";

			if( childpath.compare( other_childpath ) < 0  ) {
				// cerr << "break " << endl;
				break;
			}
			if( childpath.compare( other_childpath ) > 0  ) {
				string result = (*other_cit)->serialize(true);
				diff.append(result);
			}
			else {
				// cerr << " doing diff: " << endl;
				// other_cit is either not present in cit or just differs
				string result = (*cit)->diff(*other_cit);
				diff.append(result);
			}
			other_cit++;
		}
	}

	while( other_cit != other->m_sub_categories.end()) {
		string other_childpath = (*other_cit)->getPath();

		// cerr << "later while: other cit: " << other_childpath << " ";
		// cerr << " doing diff anyway: " << endl;
		// other_cit is either not present in cit or just differs
		string result = (*other_cit)->serialize(true);
		diff.append(result);

		other_cit++;
	}


	string ltext, rtext;
	vector<IContentItem*>::const_iterator mit;
	vector<IContentItem*>::const_iterator other_mit;

	for(mit = m_content_items.begin(); mit != m_content_items.end(); mit++) {
		ltext.append((*mit)->getText());
	}

	for(other_mit = other->m_content_items.begin(); other_mit != other->m_content_items.end(); other_mit++) {
		rtext.append((*other_mit)->getText());
	}

	if(ltext.compare(rtext) != 0 ) {
		ostringstream oss;
		oss << "+++ " << getPath() << endl;
		diff.append(oss.str());
		CDiff differ;
		string result;
		result = differ.diff(ltext, rtext);
		diff.append(result);
	}
	return diff;
}


bool CCategoryItem::operator==(const CCategoryItem& other) {

	if(m_path != other.m_path) {
		return false;
	}

	if(m_name != other.m_name) {
		return false;
	}

	if( m_sub_categories.size() != other.m_sub_categories.size() ) {
		return false;
	}

	if( m_content_items.size() != other.m_content_items.size() ) {
		return false;
	}

	vector<CCategoryItem*>::const_iterator cit = m_sub_categories.begin();
	vector<CCategoryItem*>::const_iterator other_cit = other.m_sub_categories.begin();
	while( cit != m_sub_categories.end() && other_cit != other.m_sub_categories.end()) {
		if( *(*cit) != *(*other_cit) ) {
			return false;
		}
		cit++;
		other_cit++;
	}

	vector<IContentItem*>::const_iterator mit = m_content_items.begin();
	vector<IContentItem*>::const_iterator other_mit = other.m_content_items.begin();
	while( mit != m_content_items.end() && other_mit != other.m_content_items.end()) {
		if( *(*mit) != *(*other_mit) ) {
			return false;
		}
		mit++;
		other_mit++;
	}
	return true;
}

string CCategoryItem::getParentPath(string ownPath) {
	size_t rpos = ownPath.rfind('\t', 0);
	if(rpos == string::npos ) {
		return "";
	}
	else {
		return ownPath.substr(0, rpos);
	}
}

} // namespace muroa

