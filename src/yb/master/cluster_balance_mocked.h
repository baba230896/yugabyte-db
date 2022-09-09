// Copyright (c) YugaByte, Inc.

#ifndef YB_MASTER_CLUSTER_BALANCE_MOCKED_H
#define YB_MASTER_CLUSTER_BALANCE_MOCKED_H

#include "yb/master/cluster_balance.h"
#include "yb/master/cluster_balance_util.h"
#include "yb/master/ts_manager.h"

namespace yb {
namespace master {

class ClusterLoadBalancerMocked : public ClusterLoadBalancer {
 public:
  explicit ClusterLoadBalancerMocked(Options* options) : ClusterLoadBalancer(nullptr)  {
    const int kHighNumber = 100;
    options->kMaxConcurrentAdds = kHighNumber;
    options->kMaxConcurrentRemovals = kHighNumber;
    options->kAllowLimitStartingTablets = false;
    options->kAllowLimitOverReplicatedTablets = false;

    auto table_state = std::make_unique<PerTableLoadState>(global_state_.get());
    table_state->options_ = options;
    state_ = table_state.get();
    per_table_states_[""] = std::move(table_state);

    ResetOptions();

    InitTablespaceManager();
  }

  // Overrides for base class functionality to bypass calling CatalogManager.
  void GetAllReportedDescriptors(TSDescriptorVector* ts_descs) const override {
    *ts_descs = ts_descs_;
  }

  void GetAllAffinitizedZones(
      const ReplicationInfoPB& replication_info,
      vector<AffinitizedZonesSet>* affinitized_zones) const override {
    *affinitized_zones = affinitized_zones_;
  }

  const TabletInfoMap& GetTabletMap() const override { return tablet_map_; }

  const TableInfoMap& GetTableMap() const override { return table_map_; }

  const scoped_refptr<TableInfo> GetTableInfo(const TableId& table_uuid) const override {
    return FindPtrOrNull(table_map_, table_uuid);
  }

  Result<ReplicationInfoPB> GetTableReplicationInfo(
      const scoped_refptr<const TableInfo>& table) const override {
    return replication_info_;
  }

  void SetBlacklist() const override {
    // Set the blacklist so we can also mark the tablet servers as we add them up.
    global_state_->SetBlacklist(blacklist_);

    // Set the leader blacklist so we can also mark the tablet servers as we add them up.
    global_state_->SetLeaderBlacklist(leader_blacklist_);
  }

  Status SendReplicaChanges(scoped_refptr<TabletInfo> tablet, const TabletServerId& ts_uuid,
                          const bool is_add, const bool should_remove,
                          const TabletServerId& new_leader_uuid) override {
    // Do nothing.
    return Status::OK();
  }

  void GetPendingTasks(const TableId& table_uuid,
                       TabletToTabletServerMap* pending_add_replica_tasks,
                       TabletToTabletServerMap* pending_remove_replica_tasks,
                       TabletToTabletServerMap* pending_stepdown_leader_tasks) override {
    for (const auto& tablet_id : pending_add_replica_tasks_) {
      (*pending_add_replica_tasks)[tablet_id] = "";
    }
    for (const auto& tablet_id : pending_remove_replica_tasks_) {
      (*pending_remove_replica_tasks)[tablet_id] = "";
    }
    for (const auto& tablet_id : pending_stepdown_leader_tasks_) {
      (*pending_stepdown_leader_tasks)[tablet_id] = "";
    }
  }

  void ResetTableStatePtr(const TableId& table_id, Options* options) override {
    if (state_) {
      options = state_->options_;
    }
    auto table_state = std::make_unique<PerTableLoadState>(global_state_.get());
    table_state->options_ = options;
    table_state->check_ts_liveness_ = false;
    state_ = table_state.get();

    per_table_states_[table_id] = std::move(table_state);
  }

  void InitTablespaceManager() override {
    tablespace_manager_ = std::make_shared<YsqlTablespaceManager>(nullptr, nullptr);
  }

  void SetOptions(ReplicaType type, const string& placement_uuid) {
    state_->options_->type = type;
    state_->options_->placement_uuid = placement_uuid;
  }

  void ResetOptions() { SetOptions(LIVE, ""); }

  TSDescriptorVector ts_descs_;
  vector<AffinitizedZonesSet> affinitized_zones_;
  TabletInfoMap tablet_map_;
  TableInfoMap table_map_;
  ReplicationInfoPB replication_info_;
  BlacklistPB blacklist_;
  BlacklistPB leader_blacklist_;
  vector<TabletId> pending_add_replica_tasks_;
  vector<TabletId> pending_remove_replica_tasks_;
  vector<TabletId> pending_stepdown_leader_tasks_;

  friend class TestLoadBalancerEnterprise;
};

} // namespace master
} // namespace yb

#endif // YB_MASTER_CLUSTER_BALANCE_MOCKED_H
