/*
 * CmdEditPlaylist.h
 *
 *  Created on: 28 Dec 2011
 *      Author: martin
 */

#ifndef CMD_EDIT_PLAYLIST_H_
#define CMD_EDIT_PLAYLIST_H_

#include "Cmd.h"
#include <string>

namespace muroa {

class CmdEditPlaylist: public muroa::Cmd {
public:
	CmdEditPlaylist();
	CmdEditPlaylist(unsigned  fromRev, unsigned  toRev, std::string diff);
    virtual ~CmdEditPlaylist();

    std::string getDiff() const;
    void setDiff(std::string diff);

    unsigned getFromRev() const;
    void setFromRev(unsigned  fromRev);

    unsigned getToRev() const;
    void setToRev(unsigned  fromRev);

private:
    unsigned m_fromRev;
    unsigned m_toRev;
    std::string m_diff;

};

} /* namespace muroa */
#endif /* CMD_EDIT_PLAYLIST_H_ */
