/*
 * IContentItem.h
 *
 *  Created on: 29 Jun 2011
 *      Author: martin
 */

#ifndef ICONTENTITEM_H_
#define ICONTENTITEM_H_

#include "CItemBase.h"
#include <stdexcept>
#include <MuroaExceptions.h>

namespace muroa {

class CRootItem;
class CCategoryItem;
class CMediaItem;

class IContentItem : public CItemBase {
public:
	IContentItem(CRootItem *root_item, CCategoryItem*  parent, const item_type_t type);
	IContentItem(CRootItem *root_item, std::string text, CCategoryItem*  parent, const item_type_t type, int posInParent = -1) throw(ExMalformedPatch);
	virtual ~IContentItem();

    // virtual IContentItem* clone(const IContentItem& other, CRootItem* const root_item, CCategoryItem* const  parent) = 0;

	virtual std::string serialize(bool asDiff = false) = 0;

	static IContentItem* itemFactory(const CItemType itemType,
			                         CRootItem *root_item,
			                         CCategoryItem *parent,
			                         const unsigned posInParent = -1);
	static IContentItem* itemFactory(const CItemType itemType,
			                         CRootItem *root_item,
			                         std::string text,
			                         CCategoryItem *parent,
			                         const unsigned posInParent = -1) throw(ExMalformedPatch);
	static IContentItem* itemFactory(CMediaItem* item,
			                         CRootItem* root_item,
			                         CCategoryItem *parent,
			                         const unsigned posInParent);
	static IContentItem* itemFactory(const IContentItem& other,
                                     CRootItem* const root_item,
                                     CCategoryItem* const parent);


	bool operator==(const IContentItem& other);
	inline bool operator!=(const IContentItem& other){ return !operator==(other); };

	inline uint32_t getHash() { return m_hash; };


	// these have to be implemented to fit interface of CItemBase, but do not make sense for ContentItems
	int numChildren() { return 0; };
	CItemBase* childAt(unsigned row) { return 0; };

	virtual unsigned childPos(const CItemBase* const child) {
		throw std::runtime_error("ContentItems (derived from IContentItem) may not have children!");
		return 0;
	};

	void addChild(IContentItem* /*newMediaItem*/, int /*pos*/ ) {
		throw std::runtime_error("ContentItems (derived from IContentItem) may not have children!");
	};

	static uint32_t hash( std::string stdstr );

protected:
	uint32_t m_hash;

};
} // namespace muroa
#endif /* ICONTENTITEM_H_ */
