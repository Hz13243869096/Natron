/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobSerialization.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include <ofxNatron.h>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp> // iequals
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_ENTER;

ValueSerialization::ValueSerialization()
: _serialization(0)
, _knob()
, _dimension(0)
, _master()
, _expression()
, _exprHasRetVar(false)
{

}

ValueSerialization::ValueSerialization(KnobSerializationBase* serialization,
                                       const KnobPtr & knob,
                                       int dimension)
    : _serialization(serialization)
    , _knob(knob)
    , _dimension(dimension)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
}

void
ValueSerialization::initForLoad(KnobSerializationBase* serialization,
                 const KnobPtr & knob,
                 int dimension)
{
    _serialization = serialization;
    _knob = knob;
    _dimension = dimension;
}

ValueSerialization::ValueSerialization(const KnobPtr & knob,
                                       int dimension,
                                       bool exprHasRetVar,
                                       const std::string& expr)
    : _serialization(0)
    , _knob()
    , _dimension(0)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
    initForSave(knob, dimension, exprHasRetVar, expr);
}

void
ValueSerialization::initForSave(const KnobPtr & knob,
                 int dimension,
                 bool exprHasRetVar,
                 const std::string& expr)
{
    _knob = knob;
    _dimension = dimension;
    _expression = expr;
    _exprHasRetVar = exprHasRetVar;

    std::pair< int, KnobPtr > m = knob->getMaster(dimension);

    if ( m.second && !knob->isMastersPersistenceIgnored() ) {
        _master.masterDimension = m.first;
        NamedKnobHolder* holder = dynamic_cast<NamedKnobHolder*>( m.second->getHolder() );
        assert(holder);

        TrackMarker* isMarker = dynamic_cast<TrackMarker*>(holder);
        if (isMarker) {
            _master.masterTrackName = isMarker->getScriptName_mt_safe();
            _master.masterNodeName = isMarker->getContext()->getNode()->getScriptName_mt_safe();
        } else {
            // coverity[dead_error_line]
            _master.masterNodeName = holder ? holder->getScriptName_mt_safe() : "";
        }
        _master.masterKnobName = m.second->getName();
    } else {
        _master.masterDimension = -1;
    }

}

void
ValueSerialization::setChoiceExtraLabel(const std::string& label)
{
    assert(_serialization);
    _serialization->setChoiceExtraString(label);
}

KnobPtr
KnobSerialization::createKnob(const std::string & typeName,
                              int dimension)
{
    KnobPtr ret;

    if ( typeName == KnobInt::typeNameStatic() ) {
        ret.reset( new KnobInt(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        ret.reset( new KnobBool(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        ret.reset( new KnobDouble(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        ret.reset( new KnobChoice(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobString::typeNameStatic() ) {
        ret.reset( new KnobString(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        ret.reset( new KnobParametric(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        ret.reset( new KnobColor(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        ret.reset( new KnobPath(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobLayers::typeNameStatic() ) {
        ret.reset( new KnobLayers(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        ret.reset( new KnobFile(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobOutputFile::typeNameStatic() ) {
        ret.reset( new KnobOutputFile(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobButton::typeNameStatic() ) {
        ret.reset( new KnobButton(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobSeparator::typeNameStatic() ) {
        ret.reset( new KnobSeparator(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobGroup::typeNameStatic() ) {
        ret.reset( new KnobGroup(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobPage::typeNameStatic() ) {
        ret.reset( new KnobPage(NULL, std::string(), dimension, false) );
    }

    if (ret) {
        ret->populate();
    }

    return ret;
}

static KnobPtr
findMaster(const KnobPtr & knob,
           const NodesList & allNodes,
           const std::string& masterKnobName,
           const std::string& masterNodeName,
           const std::string& masterTrackName,
           const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;
    std::string masterNodeNameToFind = masterNodeName;

    /*
       When copy pasting, the new node copied has a script-name different from what is inside the serialization because 2
       nodes cannot co-exist with the same script-name. We keep in the map the script-names mapping
     */
    std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMapping.find(masterNodeName);

    if ( foundMapping != oldNewScriptNamesMapping.end() ) {
        masterNodeNameToFind = foundMapping->second;
    }

    for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
        if ( (*it2)->getScriptName() == masterNodeNameToFind ) {
            masterNode = *it2;
            break;
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

        return KnobPtr();
    }

    if ( !masterTrackName.empty() ) {
        boost::shared_ptr<TrackerContext> context = masterNode->getTrackerContext();
        if (context) {
            TrackMarkerPtr marker = context->getMarkerByName(masterTrackName);
            if (marker) {
                return marker->getKnobByName(masterKnobName);
            }
        }
    } else {
        ///now that we have the master node, find the corresponding knob
        const std::vector< KnobPtr > & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            if ( (otherKnobs[j]->getName() == masterKnobName) && otherKnobs[j]->getIsPersistent() ) {
                return otherKnobs[j];
                break;
            }
        }
    }

    qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

    return KnobPtr();
}

void
KnobSerialization::restoreKnobLinks(const KnobPtr & knob,
                                    const NodesList & allNodes,
                                    const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int i = 0;

    if (_masterIsAlias) {
        /*
         * _masters can be empty for example if we expand a group: the slaved knobs are no longer slaves
         */
        if ( !_masters.empty() ) {
            const std::string& aliasKnobName = _masters.front().masterKnobName;
            const std::string& aliasNodeName = _masters.front().masterNodeName;
            const std::string& masterTrackName  = _masters.front().masterTrackName;
            KnobPtr alias = findMaster(knob, allNodes, aliasKnobName, aliasNodeName, masterTrackName, oldNewScriptNamesMapping);
            if (alias) {
                knob->setKnobAsAliasOfThis(alias, true);
            }
        }
    } else {
        for (std::list<MasterSerialization>::iterator it = _masters.begin(); it != _masters.end(); ++it) {
            if (it->masterDimension != -1) {
                KnobPtr master = findMaster(knob, allNodes, it->masterKnobName, it->masterNodeName, it->masterTrackName, oldNewScriptNamesMapping);
                if (master) {
                    knob->slaveTo(i, master, it->masterDimension);
                }
            }
            ++i;
        }
    }
}

void
KnobSerialization::restoreExpressions(const KnobPtr & knob,
                                      const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int dims = std::min( knob->getDimension(), _knob->getDimension() );

    try {
        for (int i = 0; i < dims; ++i) {
            if ( !_expressions[i].first.empty() ) {
                QString expr( QString::fromUtf8( _expressions[i].first.c_str() ) );

                //Replace all occurrences of script-names that we know have changed
                for (std::map<std::string, std::string>::const_iterator it = oldNewScriptNamesMapping.begin();
                     it != oldNewScriptNamesMapping.end(); ++it) {
                    expr.replace( QString::fromUtf8( it->first.c_str() ), QString::fromUtf8( it->second.c_str() ) );
                }
                knob->restoreExpression(i, expr.toStdString(), _expressions[i].second);
            }
        }
    } catch (const std::exception& e) {
        QString err = QString::fromUtf8("Failed to restore expression: %1").arg( QString::fromUtf8( e.what() ) );
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( knob->getName().c_str() ), QDateTime::currentDateTime(), err);
    }
}

void
KnobSerialization::setChoiceExtraString(const std::string& label)
{
    assert(_extraData);
    ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData);
    assert(cData);
    if (cData) {
        cData->_choiceString = label;
    }
}

static bool
startsWith(const std::string& str,
           const std::string& prefix)
{
    return str.substr( 0, prefix.size() ) == prefix;
    // case insensitive version:
    //return ci_string(str.substr(0,prefix.size()).c_str()) == prefix.c_str();
}

static bool
endsWith(const std::string &str,
         const std::string &suffix)
{
    return ( ( str.size() >= suffix.size() ) &&
            (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) );
}

static bool
containsString(const std::string& str, const std::string& substring)
{
    return str.find(substring) != std::string::npos;
}

static bool
equalsStringCaseSensitive(const std::string& str1, const std::string& str2)
{
    return str1 == str2;
}

static bool
equalsStringCaseInsensitive(const std::string& str1, const std::string& str2)
{
    return boost::iequals(str1, str2);
}

typedef bool (*stringFuncPtr)(const std::string&,const std::string&);

struct FilterMatcher
{
    // The second string parameter that is passed to the nameMatcher function
    std::string nameToMatch;

    // The function used to compare the knob name to detemrine if it matches the input or not
    stringFuncPtr func;
};

struct PluginMatch
{
    // The ID of the plug-in that is concerned by this filter.
    std::string pluginID;

    // The minimal version of the plug-in to which this filter can be applied
    // If -1 the filter is always applied on version below version max
    int pluginVersionMajorMin;
    int pluginVersionMinorMin;

    // The version of the plug-in up to which the filter must be applied
    // If these values are -1, the filter is always applied above version min
    int pluginVersionMajorMax;
    int pluginVersionMinorMax;

    stringFuncPtr func;
};

struct KnobMatch
{
    // List of plug-ins to which it applies to. If empty, applies to any plug-in
    std::list<PluginMatch> plugin;
    FilterMatcher filter;
};

struct NatronVersionMatch
{
    // The version of Natron up to which the filter must be applied.
    // If these values are -1, the filter is always applied.
    int major, minor, rev;

    NatronVersionMatch()
    : major(-1)
    , minor(-1)
    , rev(-1)
    {

    }
};


struct KnobNameFilter
{

    // All elements in the string will be checked, if one is matched, the filter applies.
    std::list<KnobMatch> filters;

    // If the string is matched using nameMatcher, this is the string that should replace it
    std::string replacement;

    // The version of Natron min/max for which it applies to.
    NatronVersionMatch natronVersionMin, natronVersionMax;

    KnobNameFilter()
    : filters()
    , replacement()
    , natronVersionMin()
    , natronVersionMax()
    {

    }
};

struct KnobChoiceOptionFilter
{

    // All elements in the string will be checked, if one is matched, the filter applies.
    std::list<KnobMatch> filters;
    std::list<FilterMatcher> optionFilters;

    // If the string is matched using nameMatcher, this is the string that should replace it
    std::string replacement;

    // The version of Natron min/max for which it applies to.
    NatronVersionMatch natronVersionMin, natronVersionMax;

    KnobChoiceOptionFilter()
    : filters()
    , optionFilters()
    , replacement()
    , natronVersionMin()
    , natronVersionMax()
    {

    }
};

PluginMatch& addPluginMatch(KnobMatch& f, const std::string& pluginID, int pluginMajorMin = -1, int pluginMinorMin = -1, int pluginMajorMax = -1, int pluginMinorMax = -1)
{
    PluginMatch m = {pluginID, pluginMajorMin, pluginMinorMin, pluginMajorMax, pluginMinorMax, equalsStringCaseInsensitive};
    f.plugin.push_back(m);
    return f.plugin.back();
}


template <typename FILTER>
KnobMatch& addKnobFilter(FILTER& f, const std::string& nameToMatch, stringFuncPtr func)
{
    KnobMatch m;
    m.filter.nameToMatch = nameToMatch;
    m.filter.func = func;
    f.filters.push_back(m);
    return f.filters.back();
}

void addOptionFilter(KnobChoiceOptionFilter& f, const std::string& nameToMatch, stringFuncPtr func)
{
    FilterMatcher m = {nameToMatch, func};
    f.optionFilters.push_back(m);
}

template <typename FILTER>
void setNatronVersionMin(FILTER &f, int major = -1, int minor = -1, int rev = -1) {
    f.natronVersionMin.major = major;
    f.natronVersionMin.minor = minor;
    f.natronVersionMin.rev = rev;
}

template <typename FILTER>
void setNatronVersionMax(FILTER &f, int major = -1, int minor = -1, int rev = -1) {
    f.natronVersionMax.major = major;
    f.natronVersionMax.minor = minor;
    f.natronVersionMax.rev = rev;
}

class KnobNameFilters
{
public:

    std::vector<KnobNameFilter> filters;

    KnobNameFilters()
    {
        {
            KnobNameFilter f;
            f.replacement = kNatronOfxParamProcessR;
            addKnobFilter(f, "r", equalsStringCaseSensitive);
            addKnobFilter(f, "doRed", equalsStringCaseSensitive);
            setNatronVersionMax(f, 1);
            filters.push_back(f);
        }
        {
            KnobNameFilter f;
            f.replacement = kNatronOfxParamProcessG;
            addKnobFilter(f, "g", equalsStringCaseSensitive);
            addKnobFilter(f, "doGreen", equalsStringCaseSensitive);
            setNatronVersionMax(f, 1);
            filters.push_back(f);
        }
        {
            KnobNameFilter f;
            f.replacement = kNatronOfxParamProcessB;
            addKnobFilter(f, "b", equalsStringCaseSensitive);
            addKnobFilter(f, "doBlue", equalsStringCaseSensitive);
            setNatronVersionMax(f, 1);
            filters.push_back(f);
        }
        {
            KnobNameFilter f;
            f.replacement = kNatronOfxParamProcessA;
            addKnobFilter(f, "a", equalsStringCaseSensitive);
            addKnobFilter(f, "doAlpha", equalsStringCaseSensitive);
            setNatronVersionMax(f, 1);
            filters.push_back(f);
        }
    }
};


static const KnobNameFilters knobNameFilters = KnobNameFilters();

template <typename FILTER>
bool matchKnobFilterInternal(const FILTER& filter, const std::string& name, const std::string& pluginID, int pluginVersionMajor, int pluginVersionMinor,
                             int natronVersionMajor, int natronVersionMinor, int natronVersionRevision)
{
    assert(!filter.replacement.empty());

    // match natron version
    {
        if (natronVersionMajor != -1 && filter.natronVersionMin.major != -1 && natronVersionMajor < filter.natronVersionMin.major) {
            return false;
        }
        if (natronVersionMajor != -1 && filter.natronVersionMax.major != -1 && natronVersionMajor > filter.natronVersionMax.major) {
            return false;
        }
    }
    {
        if (natronVersionMinor != -1 && filter.natronVersionMin.minor != -1 && natronVersionMinor < filter.natronVersionMin.minor) {
            return false;
        }
        if (natronVersionMinor != -1 && filter.natronVersionMax.minor != -1 && natronVersionMinor > filter.natronVersionMax.minor) {
            return false;
        }
    }
    {
        if (natronVersionRevision != -1 && filter.natronVersionMin.rev != -1 && natronVersionRevision < filter.natronVersionMin.rev) {
            return false;
        }
        if (natronVersionRevision != -1 && filter.natronVersionMax.rev != -1 && natronVersionRevision > filter.natronVersionMax.rev) {
            return false;
        }
    }


    // match plugin
    if (!filter.filters.empty()) {
        for (std::list<KnobMatch>::const_iterator it = filter.filters.begin(); it != filter.filters.end(); ++it) {

            if (!it->plugin.empty()) {
                bool matchPlugin = false;
                for (std::list<PluginMatch>::const_iterator it2 = it->plugin.begin(); it2 != it->plugin.end(); ++it2) {
                    if (!it2->func(pluginID, it2->pluginID)) {
                        continue;
                    }
                    {
                        if (pluginVersionMajor != -1 && it2->pluginVersionMajorMin != -1 && pluginVersionMajor < it2->pluginVersionMajorMin) {
                            return false;
                        }
                        if (pluginVersionMajor != -1 && it2->pluginVersionMajorMax != -1 && pluginVersionMajor > it2->pluginVersionMajorMax) {
                            return false;
                        }
                    }
                    {
                        if (pluginVersionMinor != -1 && it2->pluginVersionMinorMin != -1 && pluginVersionMinor < it2->pluginVersionMinorMin) {
                            return false;
                        }
                        if (pluginVersionMinor != -1 && it2->pluginVersionMinorMax != -1 && pluginVersionMinor > it2->pluginVersionMinorMax) {
                            return false;
                        }
                    }

                    matchPlugin = true;
                    break;
                }
                if (!matchPlugin) {
                    continue;
                }
            }

            if (it->filter.func(name, it->filter.nameToMatch)) {
                return true;
            }
        }
        return false;
    }


    return true;
}

bool
filterKnobNameCompat(const std::string& pluginID, int pluginVersionMajor, int pluginVersionMinor,
                     int natronVersionMajor, int natronVersionMinor, int natronVersionRevision, std::string* name)
{
    for (std::size_t i = 0; i < knobNameFilters.filters.size(); ++i) {
        const KnobNameFilter& filter = knobNameFilters.filters[i];
        assert(!filter.replacement.empty());
        if (!matchKnobFilterInternal(filter, *name, pluginID, pluginVersionMajor, pluginVersionMinor, natronVersionMajor, natronVersionMinor, natronVersionRevision)) {
            continue;
        } else {
            *name = filter.replacement;
            return true;
        }
        
    }
    return false;
}

NATRON_NAMESPACE_EXIT;
