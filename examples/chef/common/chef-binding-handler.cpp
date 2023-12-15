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

#include "chef-binding-handler.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

// static bool sSwitchOnOffState = false;

void ChefBindingHandler::OnInvokeCommandFailure(BindingData & bindingData, CHIP_ERROR err)
{
    CHIP_ERROR ret;

    if (err == CHIP_ERROR_TIMEOUT && !ChefBindingHandler::GetInstance().mCaseSessionRecovered)
    {
        ChipLogProgress(NotSpecified, "Response timeout for invoked command, trying to recover CASE session.");

        // Set flag to not try recover session multiple times.
        ChefBindingHandler::GetInstance().mCaseSessionRecovered = true;

        // Allocate new object to make sure its life time will be appropriate.
        ChefBindingHandler::BindingData * data = Platform::New<ChefBindingHandler::BindingData>();
        *data                              = bindingData;

        // Establish new CASE session and retrasmit command that was not applied.
        ret = BindingManager::GetInstance().NotifyBoundClusterChanged(bindingData.EndpointId, bindingData.ClusterId,
                                                                        static_cast<void *>(data));

        if (CHIP_NO_ERROR != ret)
        {
            ChipLogError(NotSpecified, "NotifyBoundClusterChanged failed due to: %" CHIP_ERROR_FORMAT, ret.Format());
            return;
        }
    }
    else
    {
        ChipLogError(NotSpecified, "Binding command was not applied! Reason: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

void ChefBindingHandler::OnOffProcessCommand(CommandId commandId, const EmberBindingTableEntry & bindingEntry,
                                         OperationalDeviceProxy * device, void * context)
{
    CHIP_ERROR ret     = CHIP_NO_ERROR;
    BindingData * data = reinterpret_cast<BindingData *>(context);

    auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "OnOff command succeeds");
        // If session was recovered and communication works, reset flag to the initial state.
        if (ChefBindingHandler::GetInstance().mCaseSessionRecovered)
            ChefBindingHandler::GetInstance().mCaseSessionRecovered = false;
    };

    auto onFailure = [dataRef = *data](CHIP_ERROR err) mutable {
        ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, err.Format());
	 ChefBindingHandler::OnInvokeCommandFailure(dataRef, err); 
    };

    if (device)
    {
        // We are validating connection is ready once here instead of multiple times in each case statement below.
        VerifyOrDie(device->ConnectionReady());
    }

    switch (commandId)
    {
    case Clusters::OnOff::Commands::Toggle::Id:
        Clusters::OnOff::Commands::Toggle::Type toggleCommand;
        if (device)
        {
            ret = Controller::InvokeCommandRequest(device->GetExchangeManager(), device->GetSecureSession().Value(),
                                                   bindingEntry.remote, toggleCommand, onSuccess, onFailure);
        }
        else
        {

            Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();
            ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, bindingEntry.fabricIndex, bindingEntry.groupId, toggleCommand);
        }
        break;

    case Clusters::OnOff::Commands::On::Id:
        Clusters::OnOff::Commands::On::Type onCommand;
        if (device)
        {
            ret = Controller::InvokeCommandRequest(device->GetExchangeManager(), device->GetSecureSession().Value(),
                                                   bindingEntry.remote, onCommand, onSuccess, onFailure);
        }
        else
        {
            Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();
            ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, bindingEntry.fabricIndex, bindingEntry.groupId, onCommand);
        }
        break;

    case Clusters::OnOff::Commands::Off::Id:
        Clusters::OnOff::Commands::Off::Type offCommand;
        if (device)
        {
            ret = Controller::InvokeCommandRequest(device->GetExchangeManager(), device->GetSecureSession().Value(),
                                                   bindingEntry.remote, offCommand, onSuccess, onFailure);
        }
        else
        {
            Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();
            ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, bindingEntry.fabricIndex, bindingEntry.groupId, offCommand);
        }
        break;
    default:
        ChipLogError(NotSpecified, "Invalid binding command data - commandId is not supported");
        break;
    }
    if (CHIP_NO_ERROR != ret)
    {
        ChipLogError(NotSpecified, "Invoke OnOff Command Request ERROR: %s", ErrorStr(ret));
    }
}

void ChefBindingHandler::LevelControlProcessCommand(CommandId commandId, const EmberBindingTableEntry & bindingEntry,
                                                OperationalDeviceProxy * device, void * context)
{
    BindingData * data = reinterpret_cast<BindingData *>(context);

    auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
        ChipLogProgress(NotSpecified, "LevelControlProcessCommand Binding was successful");

        // If session was recovered and communication works, reset flag to the initial state.
        if (ChefBindingHandler::GetInstance().mCaseSessionRecovered)
            ChefBindingHandler::GetInstance().mCaseSessionRecovered = false;
    };

    auto onFailure = [dataRef = *data](CHIP_ERROR err) mutable {
        ChipLogError(NotSpecified, "LevelControlProcessCommand failed: %" CHIP_ERROR_FORMAT, err.Format());
    };

    CHIP_ERROR ret = CHIP_NO_ERROR;

    if (device)
    {
        // We are validating connection is ready once here instead of multiple times in each case statement below.
        VerifyOrDie(device->ConnectionReady());
    }

    switch (commandId)
    {
    case Clusters::LevelControl::Commands::MoveToLevel::Id: {
        Clusters::LevelControl::Commands::MoveToLevel::Type moveToLevelCommand;
        moveToLevelCommand.level = data->Value;
        if (device)
        {
            ret = Controller::InvokeCommandRequest(device->GetExchangeManager(), device->GetSecureSession().Value(),
                                                   bindingEntry.remote, moveToLevelCommand, onSuccess, onFailure);
        }
        else
        {
            Messaging::ExchangeManager & exchangeMgr = Server::GetInstance().GetExchangeManager();
            ret = Controller::InvokeGroupCommandRequest(&exchangeMgr, bindingEntry.fabricIndex, bindingEntry.groupId, moveToLevelCommand);
        }
    }
    break;
    default:
        ChipLogError(NotSpecified, "Invalid binding command data - commandId is not supported");
        break;
    }
    if (CHIP_NO_ERROR != ret)
    {
        ChipLogError(NotSpecified, "Invoke Level Command Request ERROR: %s", ErrorStr(ret));
    }
}

void ChefBindingHandler::BoundDeviceChangedHandler(const EmberBindingTableEntry & bindingEntry, 
			OperationalDeviceProxy * deviceProxy, void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "Invalid context for Light switch handler"););
    BindingData * data = static_cast<BindingData *>(context);

printf("\033[41m %s, %d \033[0m  \n", __func__, __LINE__);

    if (bindingEntry.type == EMBER_MULTICAST_BINDING && data->IsGroup)
    {
        switch (data->ClusterId)
        {
        case Clusters::OnOff::Id:
            OnOffProcessCommand(data->CommandId, bindingEntry, nullptr, context);
            break;
        case Clusters::LevelControl::Id:
            LevelControlProcessCommand(data->CommandId, bindingEntry, nullptr, context);
            break;
        default:
            ChipLogError(NotSpecified, "Invalid binding group command data");
            break;
        }
    }
    else if (bindingEntry.type == EMBER_UNICAST_BINDING && !data->IsGroup)
    {
        switch (data->ClusterId)
        {
        case Clusters::OnOff::Id:
            OnOffProcessCommand(data->CommandId, bindingEntry, deviceProxy, context);
            break;
        case Clusters::LevelControl::Id:
            LevelControlProcessCommand(data->CommandId, bindingEntry, deviceProxy, context);
            break;
        default:
            ChipLogError(NotSpecified, "Invalid binding unicast command data");
            break;
        }
    }
}

void ChefBindingHandler::BoundDeviceContextReleaseHandler(void * context)
{
    VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "Invalid context for Light switch context release handler"););

    Platform::Delete(static_cast<BindingData *>(context));
}

void ChefBindingHandler::InitInternal(intptr_t aArg)
{
    static bool gInited = false;

    if (gInited) {
        ChipLogProgress(NotSpecified, "ChefBindingHandler Inited");
        return;
    }
    // Erwin: TBD aArg is endpoint
    ChipLogProgress(NotSpecified, "Initialize binding Handler, endpoint=%d", static_cast<uint8_t>(aArg));

    auto & server = Server::GetInstance();
    CHIP_ERROR ret = BindingManager::GetInstance().Init(
            { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });

    if (CHIP_NO_ERROR != ret)
    {
        ChipLogError(NotSpecified, "ChefBindingHandler::InitInternal failed: %" CHIP_ERROR_FORMAT, ret.Format());
    }

    BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(BoundDeviceChangedHandler);
    BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(BoundDeviceContextReleaseHandler);
    ChefBindingHandler::GetInstance().PrintBindingTable();
    gInited = true;
}

bool ChefBindingHandler::IsGroupBound()
{
    BindingTable & bindingTable = BindingTable::GetInstance();

    for (auto & entry : bindingTable)
    {
        if (EMBER_MULTICAST_BINDING == entry.type)
        {
            return true;
        }
    }
    return false;
}

void ChefBindingHandler::PrintBindingTable()
{
    BindingTable & bindingTable = BindingTable::GetInstance();

    ChipLogProgress(NotSpecified, "Binding Table size: [%d]:", bindingTable.Size());
    uint8_t i = 0;
    for (auto & entry : bindingTable)
    {
        switch (entry.type)
        {
        case EMBER_UNICAST_BINDING:
            ChipLogProgress(NotSpecified, "[%d] UNICAST:", i++);
            ChipLogProgress(NotSpecified, "\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ ClusterId %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ NodeId %d",
                    (int) entry.fabricIndex, (int) entry.local, (int) entry.clusterId.Value(), (int) entry.remote,
                    (int) entry.nodeId);
            break;
        case EMBER_MULTICAST_BINDING:
            ChipLogProgress(NotSpecified, "[%d] GROUP:", i++);
            ChipLogProgress(NotSpecified, "\t\t+ Fabric: %d\n \
            \t+ LocalEndpoint %d \n \
            \t+ RemoteEndpointId %d \n \
            \t+ GroupId %d",
                    (int) entry.fabricIndex, (int) entry.local, (int) entry.remote, (int) entry.groupId);
            break;
        case EMBER_UNUSED_BINDING:
            ChipLogProgress(NotSpecified, "[%d] UNUSED", i++);
            break;
        default:
            break;
        }
    }
}

void ChefBindingHandler::SwitchWorkerHandler(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "Invalid Swich data"));

    BindingData * data = reinterpret_cast<BindingData *>(context);
    ChipLogProgress(NotSpecified, "Notify Bounded Cluster | endpoint: %d cluster: %d", data->EndpointId, data->ClusterId);
    BindingManager::GetInstance().NotifyBoundClusterChanged(data->EndpointId, data->ClusterId, static_cast<void *>(data));
}

/*
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

printf("\033[41m %s, %d \033[0m  \n", __func__, __LINE__);

    if (binding.type == EMBER_UNICAST_BINDING && binding.local == 1 &&
        (!binding.clusterId.HasValue() || binding.clusterId.Value() == Clusters::OnOff::Id))
    {
        auto onSuccess = [](const ConcreteCommandPath & commandPath, const StatusIB & status, const auto & dataResponse) {
            ChipLogProgress(NotSpecified, "OnOff command succeeds");
        };
        auto onFailure = [](CHIP_ERROR ret) {
            ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, ret.Format());
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
*/

/*
static void ChefBoundDeviceContextReleaseHandler(void * context)
{
    (void) context;
}
*/
void ChefBindingHandler::Init(chip::EndpointId endpoint)
{
    DeviceLayer::PlatformMgr().ScheduleWork(InitInternal, endpoint);
}


void emberAfBindingClusterInitCallback(EndpointId endpoint)
{
printf("\033[41m %s, %d \033[0m  \n", __func__, __LINE__);

    ChefBindingHandler::GetInstance().Init(endpoint);
/*
    auto & server = chip::Server::GetInstance();
    chip::BindingManager::GetInstance().Init(
        { &server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage() });
    chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(ChefBoundDeviceChangedHandler);
    chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(ChefBoundDeviceContextReleaseHandler);
*/
}


void ChefInitiateActionSwitch(ChefBindingHandler::SwitchAction mAction)
{
    ChefBindingHandler::BindingData * data = Platform::New<ChefBindingHandler::BindingData>();
    if (data)
    {
        data->EndpointId = 1; //mLightSwitchEndpoint;
        data->ClusterId  = Clusters::OnOff::Id;
        switch (mAction)
        {
        case ChefBindingHandler::SwitchAction::Toggle:
            data->CommandId = Clusters::OnOff::Commands::Toggle::Id;
            break;
        case ChefBindingHandler::SwitchAction::On:
            data->CommandId = Clusters::OnOff::Commands::On::Id;
            break;
        case ChefBindingHandler::SwitchAction::Off:
            data->CommandId = Clusters::OnOff::Commands::Off::Id;
            break;
        default:
            Platform::Delete(data);
            return;
        }
        data->IsGroup = ChefBindingHandler::GetInstance().IsGroupBound();
        DeviceLayer::PlatformMgr().ScheduleWork(ChefBindingHandler::SwitchWorkerHandler, reinterpret_cast<intptr_t>(data));
    }
}
