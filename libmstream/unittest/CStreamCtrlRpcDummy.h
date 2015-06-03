/*
 Copyright (c) 2002-2014 "Martin Runge"

 CStreamCtrl.h is part of muroa, the  Multi Room Audio Player [http://www.muroa.org]

 Muroa is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef CSTREAMCTRL_H_
#define CSTREAMCTRL_H_

#include "ctrlrpcxml/IStreamCtrlRPC.h"
#include "ctrlrpcxml/CStreamCtrlXml.h"
namespace muroa {

class CStreamCtrlRpcDummy : public CStreamCtrlXml
{
public:
	CStreamCtrlRpcDummy();
	CStreamCtrlRpcDummy(CStreamCtrlRpcDummy* receiver);

	void setReceiver(CStreamCtrlRpcDummy* receiver) { m_receiver = receiver; };

	virtual ~CStreamCtrlRpcDummy();

	void onSetup(uint32_t cmdID);
    void onShutdown(uint32_t cmdID);

	void onAck(uint32_t cmdID);
	void onError(uint32_t cmdID, int errorCode, std::string errmsg);

	void onJoinSession(uint32_t cmdID, std::string name, boost::asio::ip::address session_srv);
	void onJoinSessionLeave();
	void onTakeFromSession(uint32_t cmdID, std::string name, boost::asio::ip::address session_srv);

	void onSetTimeSrv(uint32_t cmdID, boost::asio::ip::address session_srv, uint32_t port);
	void onGetTimeSrv(uint32_t cmdID);

	void onGetRTPPort(uint32_t cmdID);
	void onSetRTPPort(uint32_t cmdID, uint32_t port);

	void onJoinMCastGrp(uint32_t cmdID, boost::asio::ip::address mcast_addr);
	void onLeaveMCastGrp(uint32_t cmdID, boost::asio::ip::address mcast_addr);
	void onGetMCastGrp(uint32_t cmdID);

	void onSetStreamTimeBase(uint32_t cmdID, uint32_t ssrc, uint64_t rtp_ts, uint64_t pts);
	void onGetStreamTimeBase(uint32_t cmdID, uint32_t ssrc);

	void onResetStream(uint32_t cmdID, uint32_t ssrc);

	void onGetVolume(uint32_t cmdID);
	void onSetVolume(uint32_t cmdID, int percent);

	// onDataToSend:
	// called when there are serialized command ready to be sent to receiver
	void onDataToSend(const char* data, int len);

	std::string getLastCmdInternal();
	boost::asio::ip::address getLastIpAddrInternal() const;
	uint32_t getLastCmdIdInternal() const;
	uint32_t getLastSsrcInternal() const;
	uint32_t getLastPortInternal() const;
	const boost::asio::ip::address& getMcastGrpInternal() const;

	uint32_t getRtpPortInternal() const;
	void setRtpPortInternal(uint32_t port);

	uint64_t getTimebasePtsInternal() const;
	void setTimebasePtsInternal(uint64_t pts);

	uint64_t getTimebaseRtpTsInternal() const;
	void setTimebaseRtpTsInternal(uint64_t  rtp_ts);

	uint32_t getTimebaseSsrcInternal() const;
	void setTimebaseSsrcInternal(uint32_t ssrc);

	const boost::asio::ip::address& getTimesrvIpAddrInternal() const;
	void setTimesrvIpAddrInternal(const boost::asio::ip::address& ip_addr);

	uint32_t getTimesrvPortInternal() const;
	void setTimesrvPortInternal(uint32_t port);

	int getVolPercentInternal() const;
	void setVolPercentInternal(int percent);

	void setMCastGrpInternal(const boost::asio::ip::address& ip_addr);
	boost::asio::ip::address getMCastGrpInternal();

	uint32_t getLastAck() const;

private:
	CStreamCtrlRpcDummy* m_receiver;
	std::string m_last_cmd;
	uint32_t m_last_cmdID;
	uint32_t m_last_ssrc;
	uint32_t m_last_port;
	boost::asio::ip::address m_last_ip_addr;

	boost::asio::ip::address m_timesrv_ip_addr;

	boost::asio::ip::address m_mcast_addr;

	uint32_t m_timesrv_port;

	uint32_t m_rtp_port;

	boost::asio::ip::address m_mcast_grp;

	uint32_t m_timebase_ssrc;
	uint64_t m_timebase_pts;
	uint64_t m_timebase_rtp_ts;

	int m_vol_percent;

	uint32_t m_last_ack;
};

} /* namespace muroa */

#endif /* CSTREAMCTRL_H_ */
