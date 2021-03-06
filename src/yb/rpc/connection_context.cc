// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#include "yb/rpc/connection_context.h"

#include "yb/rpc/growable_buffer.h"

#include "yb/util/env.h"
#include "yb/util/mem_tracker.h"
#include "yb/util/size_literals.h"

DEFINE_int64(read_buffer_memory_limit, -5,
             "Overall limit for read buffers. "
             "Positive value - limit in bytes. "
             "Negative value - percents of RAM. "
             "Zero - unlimited.");

namespace yb {
namespace rpc {

ConnectionContextFactory::ConnectionContextFactory(
    int64_t memory_limit, const std::string& name,
    const std::shared_ptr<MemTracker>& parent_mem_tracker)
    : parent_tracker_(parent_mem_tracker) {
  int64_t root_limit = AbsRelMemLimit(FLAGS_read_buffer_memory_limit, [] {
    int64_t total_ram;
    CHECK_OK(Env::Default()->GetTotalRAMBytes(&total_ram));
    return total_ram;
  });

  auto root_buffer_tracker = MemTracker::FindOrCreateTracker(
      root_limit, "Read Buffer", parent_mem_tracker);
  memory_limit = AbsRelMemLimit(memory_limit, [&root_buffer_tracker] {
    return root_buffer_tracker->limit();
  });
  buffer_tracker_ = MemTracker::FindOrCreateTracker(memory_limit, name, root_buffer_tracker);
  auto root_call_tracker = MemTracker::FindOrCreateTracker("Call", parent_mem_tracker);
  call_tracker_ = MemTracker::FindOrCreateTracker(name, root_call_tracker);
}

ConnectionContextFactory::~ConnectionContextFactory() = default;

} // namespace rpc
} // namespace yb
