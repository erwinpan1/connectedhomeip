/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *    All rights reserved.
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
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/util/config.h>

#ifdef EMBER_AF_PLUGIN_RVC_OPERATIONAL_STATE_SERVER
#include <chef-rvc-operational-state-delegate.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OperationalState;
using namespace chip::app::Clusters::RvcOperationalState;

CHIP_ERROR GenericOperationalStateDelegateImpl::GetOperationalStateAtIndex(size_t index, GenericOperationalState & operationalState)
{
    if (index >= mOperationalStateList.size())
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    operationalState = mOperationalStateList[index];
    return CHIP_NO_ERROR;
}

CHIP_ERROR GenericOperationalStateDelegateImpl::GetOperationalPhaseAtIndex(size_t index, GenericOperationalPhase & operationalPhase)
{
    if (index >= mOperationalPhaseList.size())
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    operationalPhase = mOperationalPhaseList[index];
    return CHIP_NO_ERROR;
}

void GenericOperationalStateDelegateImpl::HandlePauseStateCallback(GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kPaused));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void GenericOperationalStateDelegateImpl::HandleResumeStateCallback(GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kRunning));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void GenericOperationalStateDelegateImpl::HandleStartStateCallback(GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kRunning));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

void GenericOperationalStateDelegateImpl::HandleStopStateCallback(GenericOperationalError & err)
{
    // placeholder implementation
    auto error = GetInstance()->SetOperationalState(to_underlying(OperationalStateEnum::kStopped));
    if (error == CHIP_NO_ERROR)
    {
        err.Set(to_underlying(ErrorStateEnum::kNoError));
    }
    else
    {
        err.Set(to_underlying(ErrorStateEnum::kUnableToCompleteOperation));
    }
}

// Init Operational State cluster

static OperationalState::Instance * gOperationalStateInstance = nullptr;
static OperationalStateDelegate * gOperationalStateDelegate   = nullptr;

void OperationalState::Shutdown()
{
    if (gOperationalStateInstance != nullptr)
    {
        delete gOperationalStateInstance;
        gOperationalStateInstance = nullptr;
    }
    if (gOperationalStateDelegate != nullptr)
    {
        delete gOperationalStateDelegate;
        gOperationalStateDelegate = nullptr;
    }
}

void emberAfOperationalStateClusterInitCallback(chip::EndpointId endpointId)
{
    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gOperationalStateInstance == nullptr && gOperationalStateDelegate == nullptr);

    gOperationalStateDelegate           = new OperationalStateDelegate;
    EndpointId operationalStateEndpoint = 0x01;
    gOperationalStateInstance = new Instance(gOperationalStateDelegate, operationalStateEndpoint, Clusters::OperationalState::Id);

    gOperationalStateInstance->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));

    gOperationalStateInstance->Init();
}

// Init RVC Operational State cluster

static OperationalState::Instance * gRvcOperationalStateInstance  = nullptr;
static RvcOperationalStateDelegate * gRvcOperationalStateDelegate = nullptr;

void RvcOperationalState::Shutdown()
{
    if (gRvcOperationalStateInstance != nullptr)
    {
        delete gRvcOperationalStateInstance;
        gRvcOperationalStateInstance = nullptr;
    }
    if (gRvcOperationalStateDelegate != nullptr)
    {
        delete gRvcOperationalStateDelegate;
        gRvcOperationalStateDelegate = nullptr;
    }
}

EmberAfStatus chefRvcOperationalStateWriteCallback(chip::EndpointId endpointId, chip::ClusterId clusterId,
                                          const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer)
{
    EmberAfStatus ret = EMBER_ZCL_STATUS_SUCCESS;

    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gRvcOperationalStateInstance != nullptr);

    chip::AttributeId attributeId    = attributeMetadata->attributeId;

    switch (attributeId)
    {
    case chip::app::Clusters::RvcOperationalState::Attributes::CurrentPhase::Id: {
        uint8_t m                           = static_cast<uint8_t>(buffer[0]);
	DataModel::Nullable<uint8_t> aPhase(m);
        CHIP_ERROR err = gRvcOperationalStateInstance->SetCurrentPhase(aPhase);
        if (CHIP_NO_ERROR == err)
        {
            break;
        }
        ret = EMBER_ZCL_STATUS_UNSUPPORTED_WRITE;
        ChipLogError(DeviceLayer, "Invalid Attribute Update status: %" CHIP_ERROR_FORMAT, err.Format());
    }
    break;
    case chip::app::Clusters::RvcOperationalState::Attributes::OperationalState::Id: {
        uint8_t m                           = static_cast<uint8_t>(buffer[0]);
        CHIP_ERROR err = gRvcOperationalStateInstance->SetOperationalState(m);
        if (CHIP_NO_ERROR == err)
        {
            break;
        }
        ret = EMBER_ZCL_STATUS_UNSUPPORTED_WRITE;
        ChipLogError(DeviceLayer, "Invalid Attribute Update status: %" CHIP_ERROR_FORMAT, err.Format());
    }
    break;
    default:
        ret = EMBER_ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        ChipLogError(DeviceLayer, "Unsupported Attribute ID: %d", static_cast<int>(attributeId));
        break;
    }

    return ret;
}

EmberAfStatus chefRvcOperationalStateReadCallback(chip::EndpointId endpoint, chip::ClusterId clusterId,
                                         const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                         uint16_t maxReadLength)
{
    EmberAfStatus ret = EMBER_ZCL_STATUS_SUCCESS;

    return ret;
}

void emberAfRvcOperationalStateClusterInitCallback(chip::EndpointId endpointId)
{
    VerifyOrDie(endpointId == 1); // this cluster is only enabled for endpoint 1.
    VerifyOrDie(gRvcOperationalStateInstance == nullptr && gRvcOperationalStateDelegate == nullptr);

    gRvcOperationalStateDelegate        = new RvcOperationalStateDelegate;
    EndpointId operationalStateEndpoint = 0x01;
    gRvcOperationalStateInstance =
        new Instance(gRvcOperationalStateDelegate, operationalStateEndpoint, Clusters::RvcOperationalState::Id);

    gRvcOperationalStateInstance->SetOperationalState(to_underlying(OperationalState::OperationalStateEnum::kStopped));

    gRvcOperationalStateInstance->Init();
}
#endif // EMBER_AF_PLUGIN_RVC_OPERATIONAL_STATE_SERVER
