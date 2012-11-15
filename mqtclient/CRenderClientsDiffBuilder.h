/*
 * CDiffBuilder.h
 *
 *  Created on: 16 Feb 2010
 *      Author: martin
 */

#ifndef CRENDERCLIENTSDIFFBUILDER_H_
#define CRENDERCLIENTSDIFFBUILDER_H_

#include <QObject>
#include <QDebug>

#include <CRootItem.h>
#include "CModelDiff.h"
#include "cmds/CmdBase.h"

namespace muroa {
  class CStreamClientsItem;
}


class CRenderClientsDiffBuilder : public QObject {
	Q_OBJECT;

public:
	CRenderClientsDiffBuilder( muroa::CRootItem* sstPtr );
	virtual ~CRenderClientsDiffBuilder();

	std::string diff(CModelDiff md);

signals:
	void sendCommand(CmdBase* cmd);


private:
	muroa::CRootItem* m_sessionStatePtr;

	muroa::CCategoryItem* m_avail_clients;
	muroa::CCategoryItem* m_own_clients;


	typedef std::string (CRenderClientsDiffBuilder::*getItemPtr)(comb_hash_t);

	CRenderClientsDiffBuilder::getItemPtr getItemToRemove;
	CRenderClientsDiffBuilder::getItemPtr getItemToInsert;

	std::string insertInOwnList(comb_hash_t comb_hash);
	std::string insertInAvailList(comb_hash_t comb_hash);
	std::string removeFromOwnList(comb_hash_t comb_hash);
	std::string removeFromAvailList(comb_hash_t comb_hash);

	void prepareDiff(CModelDiff* md);
	std::string prepareDiffHeader(unsigned minusPos, unsigned minusNum, unsigned plusPos, unsigned plusNum);

	std::string dummy(comb_hash_t combhash);

	int addFunc(int a, int b);
};

#endif /* CRENDERCLIENTSDIFFBUILDER_H_ */
