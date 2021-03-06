#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rtp/RtcpProcessor.h>
#include <rtp/RtcpProcessorHandler.h>
#include <rtp/RtpHeaders.h>
#include <MediaDefinitions.h>
#include <WebRtcConnection.h>

#include <queue>
#include <string>
#include <vector>

#include "../utils/Mocks.h"
#include "../utils/Tools.h"
#include "../utils/Matchers.h"

using ::testing::_;
using ::testing::IsNull;
using ::testing::Eq;
using ::testing::Args;
using ::testing::Return;
using erizo::dataPacket;
using erizo::packetType;
using erizo::AUDIO_PACKET;
using erizo::VIDEO_PACKET;
using erizo::IceConfig;
using erizo::RtpMap;
using erizo::RtcpProcessorHandler;
using erizo::WebRtcConnection;
using erizo::Pipeline;
using erizo::InboundHandler;
using erizo::OutboundHandler;
using erizo::Worker;
using std::queue;


class RtcpProcessorHandlerTest : public erizo::HandlerTest {
 public:
  RtcpProcessorHandlerTest() {}

 protected:
  void setHandler() {
    processor = std::make_shared<erizo::MockRtcpProcessor>();
    processor_handler = std::make_shared<RtcpProcessorHandler>(connection.get(), processor);
    pipeline->addBack(processor_handler);
  }

  std::shared_ptr<RtcpProcessorHandler> processor_handler;
  std::shared_ptr<erizo::MockRtcpProcessor> processor;
};

TEST_F(RtcpProcessorHandlerTest, basicBehaviourShouldReadPackets) {
    auto packet = erizo::PacketTools::createDataPacket(erizo::kArbitrarySeqNumber, VIDEO_PACKET);

    EXPECT_CALL(*processor, checkRtcpFb()).Times(1);
    EXPECT_CALL(*reader.get(), read(_, _)).
      With(Args<1>(erizo::RtpHasSequenceNumber(erizo::kArbitrarySeqNumber))).Times(1);

    pipeline->read(packet);
}

TEST_F(RtcpProcessorHandlerTest, basicBehaviourShouldWritePackets) {
    auto packet = erizo::PacketTools::createDataPacket(erizo::kArbitrarySeqNumber, VIDEO_PACKET);

    EXPECT_CALL(*writer.get(), write(_, _)).
      With(Args<1>(erizo::RtpHasSequenceNumber(erizo::kArbitrarySeqNumber))).Times(1);

    pipeline->write(packet);
}

TEST_F(RtcpProcessorHandlerTest, shouldWriteRTCPIfProcessorAcceptsIt) {
    uint ssrc = connection->getVideoSourceSSRC();
    uint source_ssrc = connection->getVideoSinkSSRC();
    auto packet = erizo::PacketTools::createReceiverReport(ssrc, source_ssrc, erizo::kArbitrarySeqNumber, VIDEO_PACKET);

    EXPECT_CALL(*processor, analyzeFeedback(_, _)).Times(1).WillOnce(Return(1));

    EXPECT_CALL(*writer.get(), write(_, _)).Times(1);

    pipeline->write(packet);
}

TEST_F(RtcpProcessorHandlerTest, shouldNotWriteRTCPIfProcessorRejectsIt) {
    uint ssrc = connection->getVideoSourceSSRC();
    uint source_ssrc = connection->getVideoSinkSSRC();
    auto packet = erizo::PacketTools::createReceiverReport(ssrc, source_ssrc, erizo::kArbitrarySeqNumber, VIDEO_PACKET);

    EXPECT_CALL(*processor, analyzeFeedback(_, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*writer.get(), write(_, _)).Times(0);

    pipeline->write(packet);
}

TEST_F(RtcpProcessorHandlerTest, shouldCallAnalyzeSrWhenReceivingSenderReports) {
    uint ssrc = connection->getVideoSourceSSRC();
    auto packet = erizo::PacketTools::createSenderReport(ssrc, VIDEO_PACKET);

    EXPECT_CALL(*processor, analyzeSr(_)).Times(1);
    EXPECT_CALL(*processor, checkRtcpFb()).Times(1);
    EXPECT_CALL(*reader.get(), read(_, _)).Times(1);

    pipeline->read(packet);
}

TEST_F(RtcpProcessorHandlerTest, shouldCallSetPublisherBWWhenBitrateIsCalculated) {
    uint32_t arbitraryBitrate = 10000;
    auto packet = erizo::PacketTools::createDataPacket(erizo::kArbitrarySeqNumber, VIDEO_PACKET);

    connection->getStats().setLatestTotalBitrate(arbitraryBitrate);

    EXPECT_CALL(*processor, checkRtcpFb()).Times(1);
    EXPECT_CALL(*reader.get(), read(_, _)).
      With(Args<1>(erizo::RtpHasSequenceNumber(erizo::kArbitrarySeqNumber))).Times(1);
    EXPECT_CALL(*processor, setPublisherBW(arbitraryBitrate)).Times(1);

    pipeline->read(packet);
}
