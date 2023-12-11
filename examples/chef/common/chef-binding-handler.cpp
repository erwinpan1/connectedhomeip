/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "lib/core/CHIPError.h"
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>
#include <app/CommandSender.h>
#include <app/clusters/bindings/BindingManager.h>
#include <app/server/Server.h>
#include <controller/InvokeInteraction.h>
#include <lib/core/CHIPError.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

static bool sSwitchOnOffState = false;

static void ChefBoundDeviceChangedHandler(const EmberBindingTableEntry & binding, chip::OperationalDeviceProxy * peer_device,
                                      void * context)
{
    using namespace chip;
    using namespace chip::app;

    if (binding.type == EMBER_MULTICAST_BINDING)
    {
        ChipLogError(NotSpecified, "Group binding is not supported now");
        return;
    }

    if (binding.type == EMBER_UNICAST_BINDING && binding.local == 1 &&
        (!binding.clusterId.HasValue() || binding.clusterId.Value() == Clusters::OnOff::Id))
    {
        auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
            ChipLogProgress(NotSpecified, "OnOff command succeeds");
        };
        auto onFailure = [](CHIP_ERROR error) {
            ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, error.Format());
        };

        VerifyOrDie(peer_device != nullptr && peer_device->ConnectionReady());
        if (sSwitchOnOffState)
        {
            Clusters::OnOff::Commands::On::Type onCommand;
            Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(),
                                             binding.remote, onCommand, onSuccess, onFailure);
        }
        else
        {
            Clusters::OnOff::Commands::Off::Type offCommand;
            Controller::InvokeCommandRequest(peer_device->GetExchangeManager(), peer_device->GetSecureSession().Value(),
                                             binding.remote, offCommand, onSuccess, onFailure);
        }
    }
}

static void ChefBoundDeviceContextReleaseHandler(void * context)
{
    (void) context;
}

void emberAfBindingClusterInitCallback(EndpointId endpoint)
{

    auto & server = chip::Server::GetInstance();
    chip::BindingManager::GetInstance().Init(
        { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });
    chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(ChefBoundDeviceChangedHandler);
    chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(ChefBoundDeviceContextReleaseHandler);
}
