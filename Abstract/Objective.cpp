/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * This file is part of Vega.
 *
 *   Vega is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Vega is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Vega.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Objective.cpp
 *
 *  Created on: 6 mars 2014
 *      Author: siavelis
 */

#include "Objective.h"
#include "Model.h"

using namespace std;

namespace vega {

Objective::Objective(const Model& model, Objective::Type type, int original_id) :
        Identifiable(original_id), model(model), type(type) {
}

const string Objective::name = "Objective";

const map<Objective::Type, string> Objective::stringByType = {
        { NODAL_DISPLACEMENT_ASSERTION, "NODAL_DISPLACEMENT_ASSERTION" },
        { FREQUENCY_ASSERTION, "FREQUENCY_ASSERTION" },
        { FREQUENCY_TARGET, "FREQUENCY_TARGET" },
        { FREQUENCY_BAND, "FREQUENCY_BAND" },
        { MODAL_DAMPING, "MODAL_DAMPING" },
        { NONLINEAR_STRATEGY, "NONLINEAR_STRATEGY" }
};

ostream &operator<<(ostream &out, const Objective& objective) {
    out << to_str(objective);
    return out;
}

Assertion::Assertion(const Model& model, Type type, double tolerance, int original_id) :
        Objective(model, type, original_id), tolerance(tolerance) {
}

NodalAssertion::NodalAssertion(const Model& model, Type type, double tolerance, int nodeId,
        DOF dof, int original_id) :
        Assertion(model, type, tolerance, original_id), nodePosition(model.mesh->findOrReserveNode(nodeId)), dof(dof) {
}

const DOFS NodalAssertion::getDOFSForNode(const int nodePosition) const {
    UNUSEDV(nodePosition);
    return dof;
}
set<int> NodalAssertion::nodePositions() const {
    set<int> result;
    result.insert(nodePosition);
    return result;
}

NodalDisplacementAssertion::NodalDisplacementAssertion(const Model& model, double tolerance,
        int nodeId, DOF dof, double value, double instant, int original_id) :
        NodalAssertion(model, NODAL_DISPLACEMENT_ASSERTION, tolerance, nodeId, dof,
                original_id), value(value), instant(instant) {
}

ostream &operator<<(ostream &out, const NodalDisplacementAssertion& objective) {
    out << to_str(objective) << "Node Pos " << objective.nodePosition << " DOF " << objective.dof << " "
            << objective.value;
    return out;
}

NodalComplexDisplacementAssertion::NodalComplexDisplacementAssertion(const Model& model,
        double tolerance, int nodeId, DOF dof, complex<double> value, double frequency,
        int original_id) :
        NodalAssertion(model, NODAL_COMPLEX_DISPLACEMENT_ASSERTION, tolerance, nodeId, dof,
                original_id), value(value), frequency(frequency) {
}

ostream &operator<<(ostream &out, const NodalComplexDisplacementAssertion& objective) {
    out << to_str(objective) << "Node Pos " << objective.nodePosition << " DOF " << objective.dof << " "
            << objective.value;
    return out;
}

FrequencyAssertion::FrequencyAssertion(const Model& model, int number, double value,
        double tolerance, int original_id) :
        Assertion(model, FREQUENCY_ASSERTION, tolerance, original_id), number(number), value(value) {
}

const DOFS FrequencyAssertion::getDOFSForNode(const int nodePosition) const {
    UNUSEDV(nodePosition);
    return DOFS::NO_DOFS;
}
set<int> FrequencyAssertion::nodePositions() const {
    return set<int>();
}

shared_ptr<Objective> FrequencyAssertion::clone() const {
    return make_shared<FrequencyAssertion>(*this);
}

AnalysisParameter::AnalysisParameter(const Model& model, Type type, int original_id) :
        Objective(model, type, original_id) {
}

shared_ptr<Objective> AnalysisParameter::clone() const {
    return make_shared<AnalysisParameter>(*this);
}

FrequencyRange::FrequencyRange(const Model& model, const ValueRange& valueRange, int original_id) :
        AnalysisParameter(model, FREQUENCY_TARGET, original_id), valueRange(NamedValue::STEP_RANGE,
                Reference<NamedValue>::NO_ID, valueRange.getId()) {
}

FrequencyRange::FrequencyRange(const Model& model, int range_id, int original_id) :
        AnalysisParameter(model, FREQUENCY_TARGET, original_id), valueRange(NamedValue::STEP_RANGE,
                range_id) {
}

const shared_ptr<ValueRange> FrequencyRange::getValueRange() const {
    return dynamic_pointer_cast<ValueRange>(model.find(valueRange));
}

const ValuePlaceHolder FrequencyRange::getValueRangePlaceHolder() const {
    return ValuePlaceHolder(model, valueRange.type, valueRange.original_id, NamedValue::FREQ);
}

shared_ptr<Objective> FrequencyRange::clone() const {
    return make_shared<FrequencyRange>(*this);
}

FrequencyList::FrequencyList(const Model& model, const list<double>& values, int original_id) :
        AnalysisParameter(model, FREQUENCY_TARGET, original_id), values(values) {
}

const list<double> FrequencyList::getValues() const {
    return values;
}

shared_ptr<Objective> FrequencyList::clone() const {
    return make_shared<FrequencyList>(*this);
}

FrequencyBand::FrequencyBand(const Model& model, double lower, double upper, int num_max,
        string norm, int original_id) :
        AnalysisParameter(model, FREQUENCY_BAND, original_id), lower(lower), upper(upper), num_max(
                num_max), norm(norm) {
}

double FrequencyBand::getLower() const {
    if (is_equal(lower, Globals::UNAVAILABLE_DOUBLE)) {
        auto lower_cutoff_frequency = model.parameters.find(Model::LOWER_CUTOFF_FREQUENCY);
        if (lower_cutoff_frequency != model.parameters.end()) {
            if (model.configuration.logLevel >= LogLevel::TRACE) {
                cout << "Parameter LOWER_CUTOFF_FREQUENCY present, redefining frequency band" << endl;
            }
            return lower_cutoff_frequency->second;
        }
    }
    return lower;
}

double FrequencyBand::getUpper() const {
    if (is_equal(upper, Globals::UNAVAILABLE_DOUBLE)) {
        auto upper_cutoff_frequency = model.parameters.find(Model::UPPER_CUTOFF_FREQUENCY);
        if (upper_cutoff_frequency != model.parameters.end()) {
            if (model.configuration.logLevel >= LogLevel::TRACE) {
                cout << "Parameter UPPER_CUTOFF_FREQUENCY present, redefining frequency band" << endl;
            }
            return min(upper_cutoff_frequency->second, upper);
        }
    }
    return upper;
}

shared_ptr<Objective> FrequencyBand::clone() const {
    return make_shared<FrequencyBand>(*this);
}

ModalDamping::ModalDamping(const Model& model, const FunctionTable& function_table, int original_id) :
        AnalysisParameter(model, MODAL_DAMPING, original_id), function_table(NamedValue::FUNCTION_TABLE,
                Reference<NamedValue>::NO_ID, function_table.getId()) {
}

ModalDamping::ModalDamping(const Model& model, int function_table_original_id, int original_id) :
        AnalysisParameter(model, MODAL_DAMPING, original_id), function_table(NamedValue::FUNCTION_TABLE,
                function_table_original_id) {
}

const shared_ptr<FunctionTable> ModalDamping::getFunctionTable() const {
    return dynamic_pointer_cast<FunctionTable>(model.find(function_table));
}

const ValuePlaceHolder ModalDamping::getFunctionTablePlaceHolder() const {
    return ValuePlaceHolder(model, function_table.type, function_table.original_id, NamedValue::FREQ);
}

shared_ptr<Objective> ModalDamping::clone() const {
    return make_shared<ModalDamping>(*this);
}

NonLinearStrategy::NonLinearStrategy(const Model& model, int number_of_increments, int original_id) :
        AnalysisParameter(model, NONLINEAR_STRATEGY, original_id), number_of_increments(
                number_of_increments) {
}

shared_ptr<Objective> NonLinearStrategy::clone() const {
    return make_shared<NonLinearStrategy>(*this);
}

} /* namespace vega */

