/*
 *  Copyright (c) 2016, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <pipeline/Pipeline.h>

#include <cassert>
#include <string>

namespace erizo {

PipelineBase::ContextIterator PipelineBase::removeAt(
    const typename PipelineBase::ContextIterator& it) {
  (*it)->detachPipeline();

  const auto dir = (*it)->getDirection();
  if (dir == HandlerDir::BOTH || dir == HandlerDir::IN) {
    auto it2 = std::find(inCtxs_.begin(), inCtxs_.end(), it->get());
    assert(it2 != inCtxs_.end());
    inCtxs_.erase(it2);
  }

  if (dir == HandlerDir::BOTH || dir == HandlerDir::OUT) {
    auto it2 = std::find(outCtxs_.begin(), outCtxs_.end(), it->get());
    assert(it2 != outCtxs_.end());
    outCtxs_.erase(it2);
  }

  return ctxs_.erase(it);
}

PipelineBase& PipelineBase::removeFront() {
  if (ctxs_.empty()) {
    throw std::invalid_argument("No handlers in pipeline");
  }
  removeAt(ctxs_.begin());
  return *this;
}

PipelineBase& PipelineBase::removeBack() {
  if (ctxs_.empty()) {
    throw std::invalid_argument("No handlers in pipeline");
  }
  removeAt(--ctxs_.end());
  return *this;
}

void PipelineBase::detachHandlers() {
  for (auto& ctx : ctxs_) {
    if (ctx != owner_) {
      ctx->detachPipeline();
    }
  }
}

Pipeline::Pipeline() {}

Pipeline::~Pipeline() {
  detachHandlers();
}

void Pipeline::read(std::shared_ptr<dataPacket> packet) {
  if (!front_) {
    throw std::invalid_argument("read(): no inbound handler in Pipeline");
  }
  front_->read(packet);
}

void Pipeline::readEOF() {
  if (!front_) {
    throw std::invalid_argument("readEOF(): no inbound handler in Pipeline");
  }
  front_->readEOF();
}

void Pipeline::write(std::shared_ptr<dataPacket> packet) {
  if (!back_) {
    throw std::invalid_argument("write(): no outbound handler in Pipeline");
  }
  back_->write(packet);
}

void Pipeline::close() {
  if (!back_) {
    throw std::invalid_argument("close(): no outbound handler in Pipeline");
  }
  back_->close();
}

void Pipeline::finalize() {
  front_ = nullptr;
  if (!inCtxs_.empty()) {
    front_ = dynamic_cast<InboundLink*>(inCtxs_.front());
    for (size_t i = 0; i < inCtxs_.size() - 1; i++) {
      inCtxs_[i]->setNextIn(inCtxs_[i+1]);
    }
    inCtxs_.back()->setNextIn(nullptr);
  }

  back_ = nullptr;
  if (!outCtxs_.empty()) {
    back_ = dynamic_cast<OutboundLink*>(outCtxs_.back());
    for (size_t i = outCtxs_.size() - 1; i > 0; i--) {
      outCtxs_[i]->setNextOut(outCtxs_[i-1]);
    }
    outCtxs_.front()->setNextOut(nullptr);
  }

  if (!front_) {
    // detail::logWarningIfNotUnit<R>(
    //     "No inbound handler in Pipeline, inbound operations will throw "
    //     "std::invalid_argument");
  }
  if (!back_) {
    // detail::logWarningIfNotUnit<W>(
    //     "No outbound handler in Pipeline, outbound operations will throw "
    //     "std::invalid_argument");
  }

  for (auto it = ctxs_.rbegin(); it != ctxs_.rend(); it++) {
    (*it)->attachPipeline();
  }
}

void Pipeline::notifyUpdate() {
  for (auto it = ctxs_.rbegin(); it != ctxs_.rend(); it++) {
    (*it)->notifyUpdate();
  }
}

void Pipeline::enable(std::string name) {
  for (auto it = ctxs_.rbegin(); it != ctxs_.rend(); it++) {
    if ((*it)->getName() == name) {
      (*it)->enable();
    }
  }
}

void Pipeline::disable(std::string name) {
  for (auto it = ctxs_.rbegin(); it != ctxs_.rend(); it++) {
    if ((*it)->getName() == name) {
      (*it)->disable();
    }
  }
}

}  // namespace erizo
