///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

/// @file points/AttributeArray.cc

#include "AttributeArray.h"
#include <map>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {
namespace points {


////////////////////////////////////////


namespace {

using AttributeFactoryMap = std::map<NamePair, AttributeArray::FactoryMethod>;

struct LockedAttributeRegistry
{
    tbb::spin_mutex     mMutex;
    AttributeFactoryMap mMap;
};

// Global function for accessing the registry
LockedAttributeRegistry*
getAttributeRegistry()
{
    static LockedAttributeRegistry registry;
    return &registry;
}

} // unnamed namespace


////////////////////////////////////////

// AttributeArray::ScopedRegistryLock implementation

AttributeArray::ScopedRegistryLock::ScopedRegistryLock()
    : lock(getAttributeRegistry()->mMutex)
{
}


////////////////////////////////////////

// AttributeArray implementation


AttributeArray::Ptr
AttributeArray::create(const NamePair& type, Index length, Index stride,
    bool constantStride, const ScopedRegistryLock* lock)
{
    auto* registry = getAttributeRegistry();
    tbb::spin_mutex::scoped_lock _lock;
    if (!lock)  _lock.acquire(registry->mMutex);

    auto iter = registry->mMap.find(type);
    if (iter == registry->mMap.end()) {
        OPENVDB_THROW(LookupError,
            "Cannot create attribute of unregistered type " << type.first << "_" << type.second);
    }
    return (iter->second)(length, stride, constantStride);
}


bool
AttributeArray::isRegistered(const NamePair& type, const ScopedRegistryLock* lock)
{
    LockedAttributeRegistry* registry = getAttributeRegistry();
    tbb::spin_mutex::scoped_lock _lock;
    if (!lock)  _lock.acquire(registry->mMutex);
    return (registry->mMap.find(type) != registry->mMap.end());
}


void
AttributeArray::clearRegistry(const ScopedRegistryLock* lock)
{
    LockedAttributeRegistry* registry = getAttributeRegistry();
    tbb::spin_mutex::scoped_lock _lock;
    if (!lock)  _lock.acquire(registry->mMutex);
    registry->mMap.clear();
}


void
AttributeArray::registerType(const NamePair& type, FactoryMethod factory, const ScopedRegistryLock* lock)
{
    { // check the type of the AttributeArray generated by the factory method
        auto array = (*factory)(/*length=*/0, /*stride=*/0, /*constantStride=*/false);
        const NamePair& factoryType = array->type();
        if (factoryType != type) {
            OPENVDB_THROW(KeyError, "Attribute type " << type.first << "_" << type.second
                << " does not match the type created by the factory method "
                << factoryType.first << "_" << factoryType.second << ".");
        }
    }

    LockedAttributeRegistry* registry = getAttributeRegistry();
    tbb::spin_mutex::scoped_lock _lock;
    if (!lock)  _lock.acquire(registry->mMutex);

    registry->mMap[type] = factory;
}


void
AttributeArray::unregisterType(const NamePair& type, const ScopedRegistryLock* lock)
{
    LockedAttributeRegistry* registry = getAttributeRegistry();
    tbb::spin_mutex::scoped_lock _lock;
    if (!lock)  _lock.acquire(registry->mMutex);

    registry->mMap.erase(type);
}


void
AttributeArray::setTransient(bool state)
{
    if (state) mFlags = static_cast<uint8_t>(mFlags | Int16(TRANSIENT));
    else       mFlags = static_cast<uint8_t>(mFlags & ~Int16(TRANSIENT));
}


void
AttributeArray::setHidden(bool state)
{
    if (state) mFlags = static_cast<uint8_t>(mFlags | Int16(HIDDEN));
    else       mFlags = static_cast<uint8_t>(mFlags & ~Int16(HIDDEN));
}


void
AttributeArray::setStreaming(bool state)
{
    if (state) mFlags = static_cast<uint8_t>(mFlags | Int16(STREAMING));
    else       mFlags = static_cast<uint8_t>(mFlags & ~Int16(STREAMING));
}


void
AttributeArray::setConstantStride(bool state)
{
    if (state) mFlags = static_cast<uint8_t>(mFlags | Int16(CONSTANTSTRIDE));
    else       mFlags = static_cast<uint8_t>(mFlags & ~Int16(CONSTANTSTRIDE));
}


bool
AttributeArray::operator==(const AttributeArray& other) const
{
    this->loadData();
    other.loadData();

    if (this->mUsePagedRead != other.mUsePagedRead ||
        this->mFlags != other.mFlags) return false;
    return this->isEqual(other);
}

} // namespace points
} // namespace OPENVDB_VERSION_NAME
} // namespace openvdb

// Copyright (c) DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )