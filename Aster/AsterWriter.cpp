/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * Released under the GNU General Public License
 *
 * AsterBuilder.cpp
 *
 *  Created on: 5 mars 2013
 *      Author: dallolio
 */

#include "AsterWriter.h"
#include "build_properties.h"
#include "../Abstract/Model.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <fstream>
#include <limits>

#include <ciso646>

namespace fs = boost::filesystem;
using namespace std;

namespace vega {
namespace aster {

AsterWriter::AsterWriter() {

}

const string AsterWriter::toString() const {
	return string("AsterWriter");
}

string AsterWriter::writeModel(const shared_ptr<vega::Model> model_ptr,
		const vega::ConfigurationParameters &configuration) {
	AsterModel asterModel(*model_ptr, configuration);
//string currentOutFile = asterModel.getOutputFileName();

	string path = asterModel.configuration.outputPath;
	if (!fs::exists(path)) {
		throw iostream::failure("Directory " + path + " don't exist.");
	}

  fs::path inputFile(asterModel.configuration.inputFile);
	if (fs::exists(inputFile)) {
		fs::copy_file(inputFile, fs::absolute(path) / inputFile.filename(), fs::copy_option::overwrite_if_exists);
	}

  fs::path testFile = asterModel.configuration.resultFile;
	if (fs::exists(testFile)) {
		fs::copy_file(testFile, fs::absolute(path) / testFile.filename(), fs::copy_option::overwrite_if_exists);
	}

	string exp_path = asterModel.getOutputFileName(".export");
	string med_path = asterModel.getOutputFileName(".med");
	string comm_path = asterModel.getOutputFileName(".comm");

	model_ptr->mesh->writeMED(*model_ptr, med_path.c_str());

	ofstream comm_file_ofs;
	//comm_file_ofs.setf(ios::scientific);
 	comm_file_ofs.precision(DBL_DIG);

	ofstream exp_file_ofs;
	exp_file_ofs.open(exp_path.c_str(), ios::trunc | ios::out);
	if (!exp_file_ofs.is_open()) {
		string message = string("Can't open file ") + exp_path + " for writing.";
		throw ios::failure(message);
	}
	this->writeExport(asterModel, exp_file_ofs);
	exp_file_ofs.close();

	comm_file_ofs.open(comm_path.c_str(), ios::out | ios::trunc);

	if (!comm_file_ofs.is_open()) {
		string message = string("Can't open file ") + comm_path + " for writing.";
		throw ios::failure(message);
	}
	this->writeComm(asterModel, comm_file_ofs);
	comm_file_ofs.close();
	return exp_path;
}

void AsterWriter::writeExport(AsterModel &model, ostream& out) {
	out << "P actions make_etude" << endl;
	out << "P mem_aster 100.0" << endl;
	out << "P mode interactif" << endl;
	if (model.model.analyses.size() == 0) {
		out << "P copy_result_alarm no" << endl;
	}
	out << "P nomjob " << model.model.name << endl;
	out << "P origine Vega++ " << VEGA_VERSION_MAJOR << "." << VEGA_VERSION_MINOR << endl;
	out << "P version " << model.getAsterVersion() << endl;
	out << "A memjeveux " << model.getMemjeveux() << endl;
	out << "A tpmax " << model.getTpmax() << endl;
	out << "F comm " << model.getOutputFileName(".comm", false) << " D 1" << endl;
	out << "F mail " << model.getOutputFileName(".med", false) << " D 20" << endl;
	out << "F mess " << model.getOutputFileName(".mess", false) << " R 6" << endl;
	out << "F resu " << model.getOutputFileName(".resu", false) << " R 8" << endl;
	out << "F rmed " << model.getOutputFileName(".rmed", false) << " R 80" << endl;
	out << "R repe repe_out R 0" << endl;

}

void AsterWriter::writeImprResultats(const AsterModel& asterModel, ostream& out) {
	if (asterModel.model.analyses.size() > 0) {
		out << "IMPR_RESU(FORMAT='RESULTAT'," << endl;
		out << "          RESU=(" << endl;
		for (auto it : asterModel.model.analyses) {
			const Analysis& analysis = *it;
			switch (analysis.type) {
			case (Analysis::Type::LINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_CHAM='DEPL'," << " VALE_MAX='OUI'," << " VALE_MIN='OUI',),"
						<< endl;
				break;
			}
			case (Analysis::Type::LINEAR_MODAL): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_PARA='FREQ', TOUT_CHAM='NON')," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_DYNA_DIRECT_FREQ): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_PARA='FREQ', TOUT_CHAM='NON')," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_DYNA_MODAL_FREQ): {
				break;
			}
			case (Analysis::Type::NONLINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_CHAM='DEPL'," << " VALE_MAX='OUI'," << " VALE_MIN='OUI',),"
						<< endl;
				break;
			}
			default:
				out << "# WARN analysis " << analysis << " not supported. Skipping." << endl;
			}
		}
		out << "                )," << endl;
		out << "          );" << endl << endl;
		out << "IMPR_RESU(FORMAT='MED',UNITE=80," << endl;
		out << "          RESU=(" << endl;
		for (auto it : asterModel.model.analyses) {
			const Analysis& analysis = *it;
			switch (analysis.type) {
			case (Analysis::Type::LINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_CHAM=('DEPL'";
				if (calc_sigm) {
					out << ",'" << sigm_noeu << "'";
				}
				out << ",),)," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_MODAL): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_CHAM = 'DEPL',)," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_DYNA_DIRECT_FREQ): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", PARTIE='REEL')," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_DYNA_MODAL_FREQ): {
				out << "                _F(RESULTAT=MODES" << analysis.getId()
						<< ", NOM_CHAM = 'DEPL',)," << endl;
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", PARTIE='REEL')," << endl;
				break;
			}
			case (Analysis::Type::NONLINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ", NOM_CHAM='DEPL',)," << endl;
				break;
			}
			default:
				out << "# WARN analysis " << analysis << " not supported. Skipping." << endl;
			}
		}
		out << "                )," << endl;
		out << "          );" << endl << endl;
		for (auto it : asterModel.model.analyses) {
			const Analysis& analysis = *it;

			out << "RETB" << analysis.getId() << "=CREA_TABLE(RESU=(" << endl;
			switch (analysis.type) {
			case (Analysis::Type::LINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ",TOUT='OUI',NOM_CHAM='DEPL',TOUT_CMP='OUI')," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_MODAL): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ",TOUT='OUI',NOM_CHAM='DEPL',TOUT_CMP='OUI')," << endl;
				break;
			}
			case (Analysis::Type::LINEAR_DYNA_DIRECT_FREQ): {
			    break;
			}
			case (Analysis::Type::LINEAR_DYNA_MODAL_FREQ): {
				out << "                _F(RESULTAT=MODES" << analysis.getId()
						<< ",TOUT='OUI',NOM_CHAM='DEPL',TOUT_CMP='OUI')," << endl;
				break;
			}
			case (Analysis::Type::NONLINEAR_MECA_STAT): {
				out << "                _F(RESULTAT=RESU" << analysis.getId()
						<< ",TOUT='OUI',NOM_CHAM='DEPL',TOUT_CMP='OUI')," << endl;
				break;
			}
			default:
				out << "# WARN analysis " << analysis << " not supported. Skipping." << endl;
			}
			out << "),)" << endl << endl;

			out << "DEFI_FICHIER(ACTION='ASSOCIER'," << endl;
			out << "             UNITE=26," << endl;
			out << "             FICHIER='REPE_OUT/tbresu_" << analysis.getId() << ".csv')" << endl
					<< endl;

			out << "IMPR_TABLE(TABLE=RETB" << analysis.getId() << "," << endl;
			out << "           FORMAT='TABLEAU'," << endl;
			out << "           UNITE=26," << endl;
			out << "           SEPARATEUR=' ,'," << endl;
			out << "           TITRE='RESULTS',)" << endl << endl;

			out << "DEFI_FICHIER(ACTION='LIBERER'," << endl;
			out << "             UNITE=26)" << endl << endl;
		}

		for (auto analysis : asterModel.model.analyses) {
		    bool hasRecoveryPoints = false;
            for (auto elementSet : asterModel.model.elementSets) {
                if (not elementSet->isBeam()) continue;
                auto beam = dynamic_pointer_cast<Beam>(elementSet);
                if (beam->recoveryPoints.size() >= 1) {
                    hasRecoveryPoints = true;
                    break;
                }
            }
            if (not hasRecoveryPoints) continue;

			out << "RESU" << analysis->getId() << "=CALC_CHAMP(reuse=RESU" << analysis->getId() << ","
					<< endl;
			out << "           RESULTAT=RESU" << analysis->getId() << "," << endl;
			out << "           MODELE=MODMECA," << endl;
			out << "           CONTRAINTE =('SIPO_NOEU')," << endl;
			out << ")" << endl;

            out << "RCTB" << analysis->getId() << "=MACR_LIGN_COUPE(" << endl;
            out << "            RESULTAT=RESU" << analysis->getId() << "," << endl;
            out << "            NOM_CHAM='SIPO_NOEU'," << endl;
            out << "            MODELE=MODMECA," << endl;
            out << "            LIGN_COUPE=(" << endl;
            for (auto elementSet : asterModel.model.elementSets) {
                if (not elementSet->isBeam()) continue;
                auto beam = dynamic_pointer_cast<Beam>(elementSet);
                for (auto recoveryPoint : beam->recoveryPoints) {
                    const VectorialValue& localCoords = recoveryPoint.getLocalCoords();
                    for (const Cell& cell : beam->cellGroup->getCells()) {
                        const Node& node1 = asterModel.model.mesh->findNode(cell.nodePositions[0]);
                        const VectorialValue& globalCoords = recoveryPoint.getGlobalCoords(cell.id);
                        out << "                    _F(" << endl;
                        out << "                        INTITULE='Cell " << cell.id << " stress recovery at (local):" << localCoords << ", global:" << globalCoords << "'," << endl;
                        out << "                        NOM_CMP=('SN','SMFY','SMFZ','SVY','SVZ','SMT')," << endl;
                        out << "                        TYPE='SEGMENT'," << endl;
                        out << "                        DISTANCE_MAX=" << abs(max(localCoords.y(), localCoords.z()))*2 << "," << endl;
                        out << "                        NB_POINTS=2," << endl;
                        out << "                        COOR_ORIG=(" << globalCoords.x() << "," << globalCoords.y() << "," << globalCoords.z() << ")," << endl;
                        // TODO LD find a better solution here
                        out << "                        COOR_EXTR=(" << node1.x << "," << node1.y << "," << node1.z << ")," << endl;
                        out << "                    )," << endl;
                    }
                }
            }
            out << "                    )" << endl;
			out << "            )" << endl;

            out << "DEFI_FICHIER(ACTION='ASSOCIER'," << endl;
			out << "             UNITE=26," << endl;
			out << "             FICHIER='REPE_OUT/tbrecup_" << analysis->getId() << ".csv')" << endl << endl;

            out << "IMPR_TABLE(TABLE=RCTB" << analysis->getId() << "," << endl;
			out << "           FORMAT='TABLEAU'," << endl;
			out << "           UNITE=26," << endl;
			out << "           SEPARATEUR=' ,'," << endl;
			out << "           TITRE='RESULTS',)" << endl << endl;

            out << "DEFI_FICHIER(ACTION='LIBERER'," << endl;
			out << "             UNITE=26)" << endl << endl;
		}
	}
}

void AsterWriter::writeAnalyses(const AsterModel& asterModel, ostream& out) {
	double debut = 0;
	for (auto it : asterModel.model.analyses) {
		Analysis& analysis = *it;
		debut = writeAnalysis(asterModel, analysis, out, debut);

		if (analysis.type != Analysis::Type::LINEAR_MECA_STAT && calc_sigm) {
			out << "RESU" << analysis.getId() << "=CALC_CHAMP(reuse=RESU" << analysis.getId() << ","
					<< endl;
			out << "           RESULTAT=RESU" << analysis.getId() << "," << endl;
			out << "           MODELE=MODMECA," << endl;
			out << "           CONTRAINTE =('SIGM_ELNO')," << endl;
			out << "           FORCE = 'REAC_NODA'," << endl;
			out << ")" << endl;
		}
		vector<shared_ptr<Assertion>> assertions = analysis.getAssertions();
		if (!assertions.empty()) {
			out << "TEST_RESU(RESU = (" << endl;

			for (shared_ptr<Assertion> assertion : assertions) {
				switch (assertion->type) {
				case Objective::Type::NODAL_DISPLACEMENT_ASSERTION:
					out << "                  _F(RESULTAT=RESU" << analysis.getId() << "," << endl;
					writeNodalDisplacementAssertion(asterModel, *assertion, out);
					break;
				case Objective::Type::FREQUENCY_ASSERTION:
					out << "                  _F(RESULTAT="
							<< ((analysis.type == Analysis::Type::LINEAR_MODAL) ? "RESU" : "MODES")
							<< analysis.getId() << "," << endl;
					writeFrequencyAssertion(*assertion, out);
					break;
				case Objective::Type::NODAL_COMPLEX_DISPLACEMENT_ASSERTION:
					out << "                  _F(RESULTAT=RESU" << analysis.getId() << "," << endl;
					writeNodalComplexDisplacementAssertion(asterModel, *assertion, out);
					break;
				default:
					handleWritingError(string("Not implemented"));
				}
				out << "                     )," << endl;
			}
			out << "                  )" << endl;
			out << "          );" << endl << endl;
		}
	}
}

void AsterWriter::writeComm(const AsterModel& asterModel, ostream& out) {
	string asterVersion(asterModel.getAsterVersion());
	out << "#Vega++ version " << VEGA_VERSION_MAJOR << "." << VEGA_VERSION_MINOR << endl;
	out << "#Aster version " << asterModel.getAsterVersion() << endl;
	out << "DEBUT(PAR_LOT='NON', IGNORE_ALARM=('SUPERVIS_1'))" << endl;

	mail_name = "MAIL";
	sigm_noeu = "SIGM_NOEU";
	sigm_elno = "SIGM_ELNO";
	sief_elga = "SIEF_ELGA";

	writeLireMaillage(asterModel, out);

	writeAffeModele(asterModel, out);

	for (auto value : asterModel.model.values) {
		writeValue(*value, out);
	}

	writeMaterials(asterModel, out);

	writeAffeCaraElem(asterModel, out);

	writeAffeCharMeca(asterModel, out);

	writeDefiContact(asterModel, out);

	writeAnalyses(asterModel, out);

	writeImprResultats(asterModel, out);

	out << "FIN()" << endl;
}

void AsterWriter::writeLireMaillage(const AsterModel& asterModel, ostream& out) {
	out << mail_name << "=LIRE_MAILLAGE(FORMAT='MED',";
	if (asterModel.configuration.logLevel >= LogLevel::DEBUG) {
		out << "INFO_MED=2,VERI_MAIL=_F(VERIF='OUI',),INFO=2";
	} else {
		out << "VERI_MAIL=_F(VERIF='NON',),";
	}
	out << ");" << endl << endl;

    for (auto it : asterModel.model.constraintSets) {
		ConstraintSet& constraintSet = *it;
		if (not constraintSet.hasContacts()) {
			continue;
		}
        const set<shared_ptr<Constraint>> zones = constraintSet.getConstraintsByType(
				Constraint::Type::ZONE_CONTACT);
        if (zones.empty()) {
            continue;
        }
        out << mail_name << "=MODI_MAILLAGE(reuse="<< mail_name << ",";
        out << "MAILLAGE=" << mail_name << "," << endl;
        // TODO LD should find a better solution
        int firstNodePosition = *((*zones.begin())->nodePositions().begin());
        if (asterModel.model.mesh->findNode(firstNodePosition).dofs == DOFS::ALL_DOFS) {
            out << "         ORIE_PEAU_2D=(" << endl;
        } else {
            out << "         ORIE_PEAU_3D=(" << endl;
        }
		for (shared_ptr<Constraint> constraint : zones) {
		    shared_ptr<const ZoneContact> zone = dynamic_pointer_cast<const ZoneContact>(constraint);
		    shared_ptr<const ContactBody> master = dynamic_pointer_cast<const ContactBody>(asterModel.model.find(zone->master));
		    shared_ptr<const BoundarySurface> masterSurface = dynamic_pointer_cast<const BoundarySurface>(asterModel.model.find(master->boundary));
		    shared_ptr<const ContactBody> slave = dynamic_pointer_cast<const ContactBody>(asterModel.model.find(zone->slave));
		    shared_ptr<const BoundarySurface> slaveSurface = dynamic_pointer_cast<const BoundarySurface>(asterModel.model.find(slave->boundary));
            out << "                             _F(";
            writeCellContainer(*masterSurface, out);
            out << ")," << endl;
            out << "                             _F(";
            writeCellContainer(*slaveSurface, out);
            out << ")," << endl;

		}
		out << "                             )," << endl;
		out << "                   )" << endl << endl;
	}

	for (auto it : asterModel.model.constraintSets) {
		ConstraintSet& constraintSet = *it;
		if (not constraintSet.hasContacts()) {
			continue;
		}
        const set<shared_ptr<Constraint>> surfaces = constraintSet.getConstraintsByType(
				Constraint::Type::SURFACE_SLIDE_CONTACT);
        if (surfaces.empty()) {
            continue;
        }
        out << mail_name << "=MODI_MAILLAGE(reuse="<< mail_name << ",";
        out << "MAILLAGE=" << mail_name << "," << endl;
        out << "         ORIE_PEAU_3D=(" << endl;
		for (shared_ptr<Constraint> constraint : surfaces) {
		    shared_ptr<const SurfaceSlideContact> surface = dynamic_pointer_cast<const SurfaceSlideContact>(constraint);
		    shared_ptr<const BoundaryElementFace> masterSurface = dynamic_pointer_cast<const BoundaryElementFace>(asterModel.model.find(surface->master));
		    shared_ptr<const BoundaryElementFace> slaveSurface = dynamic_pointer_cast<const BoundaryElementFace>(asterModel.model.find(surface->slave));
            out << "                             _F(";
            out << "GROUP_MA=('"
                    << masterSurface->cellGroup->getName() << "', '"
                    << slaveSurface->cellGroup->getName()
                    << "'),";
            out << ")," << endl;
		}
		out << "                             )," << endl;
		out << "                   )" << endl << endl;
	}
}

void AsterWriter::writeAffeModele(const AsterModel& asterModel, ostream& out) {
	out << "MODMECA=AFFE_MODELE(MAILLAGE=" << mail_name << "," << endl;
	out << "                    AFFE=(" << endl;
	for (auto elementSet : asterModel.model.elementSets) {
		if (elementSet->cellGroup != nullptr) {
			out << "                          _F(GROUP_MA='" << elementSet->cellGroup->getName()
					<< "'," << endl;
			out << "                             PHENOMENE='" << asterModel.phenomene << "',"
					<< endl;

			out << "                             MODELISATION="
					<< asterModel.getModelisations(elementSet) << ")," << endl;
		} else {
			out << "#Skipping element El" << *elementSet << " because no assignment" << endl;
		}
	}
	out << "                          )," << endl;
	out << "                    );" << endl << endl;
}

string AsterWriter::writeValue(NamedValue& value, ostream& out) {
	string concept_name;

	switch (value.type) {
	case Value::Type::LIST: {
		const ListValueBase& listValue = dynamic_cast<ListValueBase&>(value);

		ostringstream list_concept_ss;
		list_concept_ss << "LST" << setfill('0') << setw(5) << listValue.getId();
		concept_name = list_concept_ss.str();
		out << concept_name << "=DEFI_LIST_REEL(" << endl;
		out << "                        VALE = (";
		if (listValue.isintegral()) {
		    const ListValue<int>& listIntValue = dynamic_cast<ListValue<int>&>(value);
            for (const int val : listIntValue.getList()) {
                out << val << ",";
            }
		} else {
		    const ListValue<double>& listDblValue = dynamic_cast<ListValue<double>&>(value);
            for (const double val : listDblValue.getList()) {
                out << val << ",";
            }
		}
        out << ")," << endl;
        out << "                        );" << endl << endl;
        break;
	}
	case Value::Type::STEP_RANGE: {
		StepRange& stepRange = dynamic_cast<StepRange&>(value);
    ostringstream list_concept_ss;
    list_concept_ss << "LST" << setfill('0') << setw(5) << stepRange.getId();
    concept_name = list_concept_ss.str();
		if (not is_equal(stepRange.end, Globals::UNAVAILABLE_DOUBLE)) {
      out << concept_name << "=DEFI_LIST_REEL(" << endl;
      out << "                        DEBUT = " << stepRange.start << "," << endl;
      out << "                        INTERVALLE = _F(JUSQU_A = " << stepRange.end << "," << endl;
      out << "                                        NOMBRE = " << stepRange.count << endl;
      out << "                                        )," << endl;
      out << "                        );" << endl << endl;
		} else {
		  out << "# Ignoring " << concept_name << " because: no end value" << endl;
		}
		break;
	}
	case Value::Type::SPREAD_RANGE: {
			SpreadRange& spreadRange = dynamic_cast<SpreadRange&>(value);
			ostringstream list_concept_ss;
			list_concept_ss << "LST" << setfill('0') << setw(5) << spreadRange.getId();
			concept_name = list_concept_ss.str();
			out << concept_name << "=DEFI_LIST_FREQ(" << endl;
			out << "                        DEBUT = " << spreadRange.start << "," << endl;
			out << "                        INTERVALLE = _F(JUSQU_A = " << spreadRange.end << "," << endl;
			out << "                                        NOMBRE = " << spreadRange.count << endl;
			out << "                                        )," << endl;
			out << "                        RAFFINEMENT = _F(LIST_RAFFINE = XXXX" << "," << endl;
			out << "                                        CRITERE = 'RELATIF'," << endl;
			out << "                                        DISPERSION = " << spreadRange.spread << endl;
			out << "                                        )," << endl;
			out << "                        );" << endl << endl;
			break;
		}
	case Value::Type::FUNCTION_TABLE: {
		FunctionTable& functionTable = dynamic_cast<FunctionTable&>(value);
		ostringstream concept_ss;
		concept_ss << "FCT" << setfill('0') << setw(5) << functionTable.getId();
		concept_name = concept_ss.str();
		out << concept_name << "=DEFI_FONCTION(" << endl;
		if (functionTable.hasParaX())
			out << "                       NOM_PARA="
					<< AsterModel::NomParaByParaName.find(functionTable.getParaX())->second << ","
					<< endl;

		if (functionTable.hasParaY())
			out << "                       NOM_RESU="
					<< AsterModel::NomParaByParaName.find(functionTable.getParaY())->second << ","
					<< endl;

		out << "                       VALE = (" << endl;
		for (auto it = functionTable.getBeginValuesXY();
				it != functionTable.getEndValuesXY(); it++)
			out << "                               " << it->first << ", " << it->second << ","
					<< endl;
		out << "                               )," << endl;
		out << "                       INTERPOL = ("
				<< AsterModel::InterpolationByInterpolation.find(functionTable.parameter)->second
				<< "," << AsterModel::InterpolationByInterpolation.find(functionTable.value)->second
				<< ")," << endl;
		out << "                       PROL_GAUCHE="
				<< AsterModel::ProlongementByInterpolation.find(functionTable.left)->second << ","
				<< endl;
		out << "                       PROL_DROITE="
				<< AsterModel::ProlongementByInterpolation.find(functionTable.right)->second << ","
				<< endl;
		out << "                       );" << "# Original id:" << functionTable.getOriginalId() << endl << endl;
		break;
	}
	case Value::Type::SCALAR:
	case Value::Type::BAND_RANGE:
	case Value::Type::DYNA_PHASE:
		break;
	default:
		handleWritingError(string("NamedValue not yet implemented"));
	}
	asternameByValue[value.getReference()] = concept_name;

	return concept_name;
}

void AsterWriter::writeMaterials(const AsterModel& asterModel, ostream& out) {

	for (auto material : asterModel.model.materials) {
		if (material->isOriginal()) {
			out << "#Material original id " << material->getOriginalId() << endl;
		}
		out << "M" << material->getId() << "=DEFI_MATERIAU(" << endl;
		const shared_ptr<Nature> enature = material->findNature(Nature::NatureType::NATURE_ELASTIC);
		if (enature) {
			const ElasticNature& elasticNature = dynamic_cast<ElasticNature&>(*enature);
			out << "                 ELAS=_F(" << endl;
			out << "                         E=" << elasticNature.getE() << "," << endl;
			out << "                         NU=" << elasticNature.getNu() << "," << endl;
			out << "                         RHO=" << elasticNature.getRho() << "," << endl;
			out << "                         )," << endl;
			if (!is_zero(elasticNature.getGE())){
                cerr <<"Warning in Material: no support for GE material properties yet.";
			}
		}
		const shared_ptr<Nature> onature = material->findNature(Nature::NatureType::NATURE_ORTHOTROPIC);
		if (onature) {
			const OrthotropicNature& orthoNature = dynamic_cast<OrthotropicNature&>(*onature);
			out << "                 ELAS_ORTH=_F(" << endl;
			out << "                         E_L=" << orthoNature.getE_longitudinal() << "," << endl;
            out << "                         E_T=" << orthoNature.getE_transverse() << "," << endl;
            out << "                         G_LT=" << orthoNature.getG_longitudinal_transverse() << "," << endl;
            if (!is_equal(orthoNature.getG_transverse_normal(),Globals::UNAVAILABLE_DOUBLE)){
                out << "                         G_TN=" << orthoNature.getG_transverse_normal() << "," << endl;
            }
            if (!is_equal(orthoNature.getG_longitudinal_normal(),Globals::UNAVAILABLE_DOUBLE)){
                out << "                         G_LN=" << orthoNature.getG_longitudinal_normal() << "," << endl;
            }
			out << "                         NU_LT=" << orthoNature.getNu_longitudinal_transverse() << "," << endl;
			out << "                         )," << endl;
		}
		const shared_ptr<Nature> binature = material->findNature(Nature::NatureType::NATURE_BILINEAR_ELASTIC);
		if (binature) {
			const BilinearElasticNature& bilinearNature =
					dynamic_cast<BilinearElasticNature&>(*binature);
			out << "                 ECRO_LINE=_F(" << endl;
			out << "                         D_SIGM_EPSI=" << bilinearNature.secondary_slope << ","
					<< endl;
			out << "                         SY=" << bilinearNature.elastic_limit << "," << endl;
			out << "                         )," << endl;
		}
		out << "                 );" << "# Original id:" << material->getOriginalId() << endl << endl;
	}

	vector<shared_ptr<ElementSet>> composites = asterModel.model.elementSets.filter(ElementSet::Type::COMPOSITE);
    out << "                    # writing " << composites.size() << " composites" << endl;
    for (shared_ptr<ElementSet> c : composites) {
        shared_ptr<Composite> composite = dynamic_pointer_cast<Composite>(c);
        out << "MC" << composite->getId() << "=DEFI_COMPOSITE(" << endl;
        out << "                 COUCHE=(" << endl;
        for (auto& layer : composite->getLayers()) {
            out << "                     _F(EPAIS=" << layer.getThickness() << ",  MATER=M" << layer.getMaterialId() << ", ORIENTATION=" << layer.getOrientation() << ")," << endl;
        }
        out << "                         )," << endl;
        out << "                 );" << endl << endl;
    }

  if (asterModel.model.materials.size() >= 1) {
    out << "CHMAT=AFFE_MATERIAU(MAILLAGE=" << mail_name << "," << endl;
    out << "                    AFFE=(" << endl;
    for (auto& material : asterModel.model.materials) {
      CellContainer cells = material->getAssignment();
      if (!cells.empty()) {
        out << "                          _F(MATER=M" << material->getId() << ",";
        writeCellContainer(cells, out);
        out << ")," << endl;
      } else {
        out << "# WARN Skipping material id " << material->getId() << " because no assignment"
            << endl;
      }
    }
    for (shared_ptr<ElementSet> c : composites) {
        shared_ptr<Composite> composite = dynamic_pointer_cast<Composite>(c);
          out << "                          _F(MATER=MC" << composite ->getId() << ", GROUP_MA='" << composite->cellGroup->getName() << "')," << endl;
    }
    out << "                          )," << endl;
    out << "                    );" << endl << endl;
  }
}

void AsterWriter::writeAffeCaraElem(const AsterModel& asterModel, ostream& out) {
	calc_sigm = false;
	if (asterModel.model.elementSets.size() > 0) {
		out << "CAEL=AFFE_CARA_ELEM(MODELE=MODMECA," << endl;

		vector<shared_ptr<ElementSet>> discrets_0d = asterModel.model.elementSets.filter(
				ElementSet::Type::DISCRETE_0D);
		vector<shared_ptr<ElementSet>> discrets_1d = asterModel.model.elementSets.filter(
				ElementSet::Type::DISCRETE_1D);
		vector<shared_ptr<ElementSet>> nodal_masses = asterModel.model.elementSets.filter(
				ElementSet::Type::NODAL_MASS);
		vector<shared_ptr<ElementSet>> scalar_springs = asterModel.model.elementSets.filter(
				ElementSet::Type::SCALAR_SPRING);
		vector<shared_ptr<ElementSet>> structural_segments = asterModel.model.elementSets.filter(
				ElementSet::Type::STRUCTURAL_SEGMENT);
        auto numDiscrets = discrets_0d.size() + nodal_masses.size() + discrets_1d.size() + structural_segments.size() + scalar_springs.size();
		out << "                    # writing " << numDiscrets << " discrets" << endl;
		if (numDiscrets > 0) {
			out << "                    DISCRET=(" << endl;
			for (shared_ptr<ElementSet> discret : discrets_0d) {
				if (discret->cellGroup != nullptr) {
					out << "                             _F(GROUP_MA='"
							<< discret->cellGroup->getName() << "'," << endl;
					shared_ptr<DiscretePoint> discret_0d = dynamic_pointer_cast<DiscretePoint>(discret);
					if (discret_0d->hasRotations())
						out << "                                CARA='K_TR_D_N', VALE=(";
					else
						out << "                                CARA='K_T_D_N', VALE=(";
					for (double rigi : discret_0d->asStiffnessVector())
						out << rigi << ",";
					out << "),)," << endl;
				} else
					out
							<< "                             # WARN Finite Element : DISCRETE_0D ignored because its GROUP_MA is empty."
							<< endl;
			}
			for (shared_ptr<ElementSet> discret : discrets_1d) {
				if (discret->cellGroup != nullptr) {
					out << "                             _F(GROUP_MA='"
							<< discret->cellGroup->getName() << "'," << endl;
					shared_ptr<DiscreteSegment> discret_1d = dynamic_pointer_cast<DiscreteSegment>(discret);
					if (discret_1d->hasRotations())
						out << "                                CARA='K_TR_L', VALE=(";
					else
						out << "                                CARA='K_T_L', VALE=(";
					for (double rigi : discret_1d->asStiffnessVector())
						out << rigi << ",";
					out << "),)," << endl;
				} else
					out
							<< "                             # WARN Finite Element : DISCRETE_1D ignored because its GROUP_MA is empty."
							<< endl;
			}
			for (shared_ptr<ElementSet> segment : structural_segments) {
				if (segment->cellGroup != nullptr) {
					shared_ptr<StructuralSegment> segment_1d = dynamic_pointer_cast<StructuralSegment>(segment);
					out << "                             _F(GROUP_MA='"
							<< segment_1d->cellGroup->getName() << "'," << endl;
					if (segment_1d->hasRotations())
						out << "                                CARA='K_TR_L', VALE=(";
					else
						out << "                                CARA='K_T_L', VALE=(";
					for (double rigi : segment_1d->asStiffnessVector())
						out << rigi << ",";
					out << "),)," << endl;

					out << "                             _F(GROUP_MA='"
							<< segment_1d->cellGroup->getName() << "'," << endl;
					if (segment_1d->hasRotations())
						out << "                                CARA='A_TR_L', VALE=(";
					else
						out << "                                CARA='A_T_L', VALE=(";
					for (double rigi : segment_1d->asDampingVector())
						out << rigi << ",";
					out << "),)," << endl;
				} else
					out
							<< "                             # WARN Finite Element : DISCRETE_1D ignored because its GROUP_MA is empty."
							<< endl;
			}
			for (shared_ptr<ElementSet> scalar_springset : scalar_springs) {
				if (scalar_springset->cellGroup != nullptr) {
					out << "                             _F(GROUP_MA='"
							<< scalar_springset->cellGroup->getName() << "'," << endl;
					shared_ptr<ScalarSpring> spring = dynamic_pointer_cast<ScalarSpring>(scalar_springset);
					if (spring->hasStiffness()) {
                        if (spring->hasRotations())
                            out << "                                CARA='K_TR_L', VALE=(";
                        else
                            out << "                                CARA='K_T_L', VALE=(";
                        for (double rigi : spring->asStiffnessVector())
                            out << rigi << ",";
                        out << "),";
                    }
					if (spring->hasDamping()) {
                        if (spring->hasRotations())
                            out << "                                CARA='A_TR_L', VALE=(";
                        else
                            out << "                                CARA='A_T_L', VALE=(";
                        for (double dampval : spring->asDampingVector())
                            out << dampval << ",";
                        out << "),";
                    }
					out << ")," << endl;
				} else
					out
							<< "                             # WARN Finite Element : DISCRETE_1D ignored because its GROUP_MA is empty."
							<< endl;
			}
			for (shared_ptr<ElementSet> discret : nodal_masses) {
				if (discret->cellGroup != nullptr) {
					out << "                             _F(GROUP_MA='"
							<< discret->cellGroup->getName() << "'," << endl;
					shared_ptr<NodalMass> nodalMass = dynamic_pointer_cast<NodalMass>(discret);
					out << "                                CARA='M_TR_D_N',VALE=("
							<< nodalMass->getMass() << "," << nodalMass->ixx << ","
							<< nodalMass->iyy << "," << nodalMass->izz << "," << nodalMass->ixy
							<< "," << nodalMass->iyz << "," << nodalMass->ixz << ","
							<< nodalMass->ex << "," << nodalMass->ey << "," << nodalMass->ez
							<< "),)," << endl;
				} else
					out
							<< "                             # WARN Finite Element : NODAL_MASS ignored because its GROUP_MA is empty."
							<< endl;
			}
			out << "                             )," << endl;
		}
		vector<shared_ptr<Beam>> poutres = asterModel.model.getBeams();

		out << "                    # writing " << poutres.size() << " poutres" << endl;
		if (poutres.size() > 0) {
			out << "                    POUTRE=(" << endl;
			for (auto poutre : poutres) {
				writeAffeCaraElemPoutre(*poutre, out);
			}
			out << "                            )," << endl;
		}
		vector<shared_ptr<ElementSet>> shells = asterModel.model.elementSets.filter(ElementSet::Type::SHELL);
		vector<shared_ptr<ElementSet>> composites = asterModel.model.elementSets.filter(ElementSet::Type::COMPOSITE);
		out << "                    # writing " << shells.size()+composites.size() << " shells (ou composites)" << endl;
		if (shells.size() + +composites.size() > 0) {
			calc_sigm = true;
			out << "                    COQUE=(" << endl;
			for (shared_ptr<ElementSet> shell : shells) {
				if (shell->cellGroup != nullptr) {
					out << "                           _F(GROUP_MA='" << shell->cellGroup->getName()
							<< "'," << endl;
					out << "                              EPAIS="
							<< dynamic_pointer_cast<Shell>(shell)->thickness << "," << endl;
					out << "                              VECTEUR=(0.9,0.1,0.2))," << endl;
				} else
					out
							<< "                           # WARN Finite Element : COQUE ignored because its GROUP_MA is empty."
							<< endl;
			}
			for (shared_ptr<ElementSet> c : composites) {
                shared_ptr<Composite> composite = dynamic_pointer_cast<Composite>(c);
				if (composite->cellGroup != nullptr) {
					out << "                           _F(GROUP_MA='" << composite->cellGroup->getName()
							<< "'," << endl;
					out << "                              EPAIS="
							<< composite->getTotalThickness() << "," << endl;
					out << "                              COQUE_NCOU="
							<< composite->getLayers().size() << "," << endl;
					out << "                              VECTEUR=(1.0,0.0,0.0))," << endl;
				} else
					out
							<< "                           # WARN Finite Element : COMPOSITE ignored because its GROUP_MA is empty."
							<< endl;
			}
			out << "                           )," << endl;
		}
		vector<shared_ptr<ElementSet>> solids = asterModel.model.elementSets.filter(
				ElementSet::Type::CONTINUUM);
		out << "                    # writing " << solids.size() << " solids" << endl;
		if (solids.size() > 0) {
			out << "                    MASSIF=(" << endl;
			for (shared_ptr<ElementSet> solid : solids) {
				if (solid->cellGroup != nullptr) {
					out << "                            _F(GROUP_MA='"
							<< solid->cellGroup->getName() << "'," << endl;
					out << "                               ANGL_REP=(0.,0.,0.,),)," << endl;
				} else
					out << "# WARN Finite Element : MASSIF ignored because its GROUP_MA is empty."
							<< endl;
			}
			out << "                            )," << endl;
		}
	}

	//orientations
	bool orientationsPrinted = false;
	for (auto it : asterModel.model.mesh->cellGroupNameByCID){
		if (!orientationsPrinted) {
			out << "                    ORIENTATION=(" << endl;
			orientationsPrinted = true;
		}
		std::shared_ptr<vega::CoordinateSystem> cs= asterModel.model.mesh->getCoordinateSystemByPosition(it.first);
		if (cs->type!=CoordinateSystem::Type::ORIENTATION){
		   handleWritingError("Coordinate System of Group "+ it.second+" is not an ORIENTATION.");
		}
		std::shared_ptr<OrientationCoordinateSystem> ocs = std::dynamic_pointer_cast<OrientationCoordinateSystem>(cs);

		out << "                                 _F(CARA ='VECT_Y',VALE=(";
		out << ocs->getV().x() << "," << ocs->getV().y() << "," << ocs->getV().z() << ")";
		out<<",GROUP_MA='"<< it.second << "')," << endl;
	}
	if (orientationsPrinted) {
		out << "                                 )," << endl;
	}
	out << "                    );" << endl << endl;
}
void AsterWriter::writeAffeCaraElemPoutre(const ElementSet& elementSet, ostream& out) {
	out << "                            _F(GROUP_MA='" << elementSet.cellGroup->getName() << "',"
			<< endl;
	switch (elementSet.type) {
	case ElementSet::Type::RECTANGULAR_SECTION_BEAM: {
		const RectangularSectionBeam& rectBeam =
				static_cast<const RectangularSectionBeam&>(elementSet);
		out << "                               VARI_SECT='CONSTANT'," << endl;
		out << "                               SECTION='RECTANGLE'," << endl;
		out << "                               CARA=('HY','HZ',)," << endl;
		out << "                               VALE=(" << rectBeam.height << "," << rectBeam.width
				<< ")," << endl;
		break;
	}
	case ElementSet::Type::CIRCULAR_SECTION_BEAM: {
		const CircularSectionBeam& circBeam = static_cast<const CircularSectionBeam&>(elementSet);
		out << "                               SECTION='CERCLE'," << endl;
		out << "                               CARA=('R',)," << endl; //{%- if element_fini.ep != None %}'EP',{%- endif -%}),
		out << "                               VALE=(" << circBeam.radius << ")," << endl; //{%- if element_fini.ep != None %}{{ element_fini.ep }},{%- endif -%}),
		break;
	}
	default:
		out << "                               SECTION='GENERALE'," << endl;
		out << "                               CARA=('A','IY','IZ','JX','AY','AZ',)," << endl;
		out << "                               VALE=(";
		const Beam& beam =
				static_cast<const Beam&>(elementSet);
		out << max(std::numeric_limits<double>::epsilon(), beam.getAreaCrossSection()) << ","
				<< max(std::numeric_limits<double>::epsilon(), beam.getMomentOfInertiaY()) << "," << max(std::numeric_limits<double>::epsilon(), beam.getMomentOfInertiaZ())
				<< "," << max(std::numeric_limits<double>::epsilon(), beam.getTorsionalConstant()) << ",";
        if (! is_zero(beam.getShearAreaFactorY()))
            out << 1.0 / beam.getShearAreaFactorY();
        else
            out << 1.0;
        out << ",";
        if (! is_zero(beam.getShearAreaFactorZ()))
            out << 1.0 / beam.getShearAreaFactorZ();
        else
            out << 1.0;
		out << ")," << endl;
	}
	out << "                               )," << endl;
}

void AsterWriter::writeAffeCharMeca(const AsterModel& asterModel, ostream& out) {
	for (auto it : asterModel.model.constraintSets) {
		ConstraintSet& constraintSet = *it;
		if (constraintSet.getConstraints().size() == 0) {
			//GC fix for http://hotline.alneos.fr/redmine/issues/801.
			//What is the meaning of an empty constraintset?
			//maybe it should be fixed elsewhere
			continue;
		}
		bool contactOnly = true;
		for(auto& constraint : constraintSet.getConstraints()) {
		    if (not constraint->isContact()) {
		        contactOnly = false;
                break;
		    }
		}
		if (contactOnly) {
			continue;
		}
		if (constraintSet.isOriginal()) {
			out << "# ConstraintSet original id : " << constraintSet.getOriginalId() << endl;
		}
		for(bool withFunctions : {false, true} ) {
            string asterName;
            if (withFunctions and constraintSet.hasFunctions()) {
                asterName = string("BLF") + to_string(constraintSet.getId());
                out << asterName << "=AFFE_CHAR_MECA_F(MODELE=MODMECA," << endl;
            } else if (not withFunctions and not constraintSet.hasFunctions()) {
                asterName = string("BL") + to_string(constraintSet.getId());
                out << asterName << "=AFFE_CHAR_MECA(MODELE=MODMECA," << endl;
            } else
                continue;
            asternameByConstraintSet[constraintSet.getReference()] = asterName;

            writeSPC(asterModel, constraintSet, out);
            writeLIAISON_SOLIDE(asterModel, constraintSet, out);
            writeRBE3(asterModel, constraintSet, out);
            writeLMPC(asterModel, constraintSet, out);
            out << "                   );" << endl << endl;
		}
	}

	for (auto it : asterModel.model.loadSets) {
		LoadSet& loadSet = *it;
		if (loadSet.type == LoadSet::Type::DLOAD) {
			continue;
		}
		if (loadSet.getLoadings().size() == 0) {
			continue;
		}
		if (loadSet.getLoadings().size()
				== loadSet.getLoadingsByType(Loading::Type::INITIAL_TEMPERATURE).size()) {
			out << "# Ignoring INITIAL_TEMPERATURES!!!!!!" << endl;
			cout << "!!!!!!Ignoring INITIAL_TEMPERATURES!!!!!!" << endl;
			continue;
		}
		for(bool withFunctions : {false, true} ) {
            string asterName;
		    if (withFunctions and loadSet.hasFunctions()) {
                asterName = string("CHMEF") + to_string(loadSet.getId());
                out << asterName << "=AFFE_CHAR_MECA_F(MODELE=MODMECA," << endl;
		    } else if (not withFunctions and not loadSet.hasFunctions()) {
		        asterName = string("CHMEC") + to_string(loadSet.getId());
                out << asterName << "=AFFE_CHAR_MECA(MODELE=MODMECA," << endl;
            } else
                continue;
            asternameByLoadSet[loadSet.getReference()] = asterName;
            writePression(loadSet, out);
            writeForceCoque(loadSet, out);
            writeNodalForce(asterModel, loadSet, out);
            writeForceSurface(loadSet, out);
            writeForceLine(loadSet, out);
            writeGravity(loadSet, out);
            writeRotation(loadSet, out);
            out << "                      );" << endl << endl;
		}
	}
}

void AsterWriter::writeDefiContact(const AsterModel& asterModel, ostream& out) {
	for (auto it : asterModel.model.constraintSets) {
		ConstraintSet& constraintSet = *it;
		if (constraintSet.getConstraints().size() == 0) {
			// LD filter empty constraintSet
			continue;
		}
		if (not constraintSet.hasContacts()) {
			continue;
		}
		const set<shared_ptr<Constraint>> gaps = constraintSet.getConstraintsByType(
				Constraint::Type::GAP);
        const set<shared_ptr<Constraint>> slides = constraintSet.getConstraintsByType(
				Constraint::Type::SLIDE);
        const set<shared_ptr<Constraint>> surfaces = constraintSet.getConstraintsByType(
				Constraint::Type::SURFACE_CONTACT);
        const set<shared_ptr<Constraint>> zones = constraintSet.getConstraintsByType(
				Constraint::Type::ZONE_CONTACT);
		for (shared_ptr<Constraint> constraint : gaps) {
			shared_ptr<const Gap> gap = dynamic_pointer_cast<const Gap>(constraint);
			int gapCount = 0;
			for (shared_ptr<Gap::GapParticipation> gapParticipation : gap->getGaps()) {
				gapCount++;
				out << "C" << constraintSet.getId() << "I" << to_string(gapCount)
						<< "=DEFI_CONSTANTE(VALE=" << gap->initial_gap_opening << ")" << endl;
				if (!is_zero(gapParticipation->direction.x())) {
					out << "C" << constraintSet.getId() << "MX" << to_string(gapCount)
							<< "=DEFI_CONSTANTE(VALE=" << gapParticipation->direction.x() << ")"
							<< endl;
				}
				if (!is_zero(gapParticipation->direction.y())) {
					out << "C" << constraintSet.getId() << "MY" << to_string(gapCount)
							<< "=DEFI_CONSTANTE(VALE=" << gapParticipation->direction.y() << ")"
							<< endl;
				}
				if (!is_zero(gapParticipation->direction.y())) {
					out << "C" << constraintSet.getId() << "MZ" << to_string(gapCount)
							<< "=DEFI_CONSTANTE(VALE=" << gapParticipation->direction.z() << ")"
							<< endl;
				}
			}
		}
		string asterName = string("CN") + to_string(constraintSet.getId());
		asternameByConstraintSet[constraintSet.getReference()] = asterName;
		out << asterName << "=DEFI_CONTACT(MODELE=MODMECA," << endl;
		if (gaps.size() >= 1) {
            out << "                   FORMULATION='LIAISON_UNIL'," << endl;
		} else if (slides.size() >= 1) {
		    out << "                   FORMULATION='CONTINUE'," << endl;
		    out << "                   FROTTEMENT='COULOMB'," << endl;
		} else if (surfaces.size() >= 1) {
		    out << "                   FORMULATION='CONTINUE'," << endl;
		}
		out << "                   ZONE=(" << endl;
		for (shared_ptr<Constraint> constraint : gaps) {
			shared_ptr<const Gap> gap = dynamic_pointer_cast<const Gap>(constraint);
			int gapCount = 0;
			for (shared_ptr<Gap::GapParticipation> gapParticipation : gap->getGaps()) {
				gapCount++;
				out << "                             _F(";
				out << "NOEUD='"
						<< Node::MedName(gapParticipation->nodePosition)
						<< "',";
				out << "COEF_IMPO=" << "C" << constraintSet.getId() << "I" << to_string(gapCount)
						<< ",";
				out << "COEF_MULT=(";
				if (!is_zero(gapParticipation->direction.x() )) {
					out << "C" << constraintSet.getId() << "MX" << to_string(gapCount) << ",";
				}
				if (!is_zero(gapParticipation->direction.y() )) {
					out << "C" << constraintSet.getId() << "MY" << to_string(gapCount) << ",";
				}
				if (!is_zero(gapParticipation->direction.z())) {
					out << "C" << constraintSet.getId() << "MZ" << to_string(gapCount);
				}
				out << "),";
				out << "NOM_CMP=(";
				if (!is_zero(gapParticipation->direction.x())) {
					out << "'DX',";
				}
				if (!is_zero(gapParticipation->direction.y() )) {
					out << "'DY',";
				}
				if (!is_zero(gapParticipation->direction.z() )) {
					out << " 'DZ'";
				}
				out << "),";
				out << ")," << endl;
			}
		}
		for (shared_ptr<Constraint> constraint : slides) {
		    shared_ptr<const SlideContact> slide = dynamic_pointer_cast<const SlideContact>(constraint);
                out << "                             _F(";
				out << "GROUP_MA_MAIT='"
						<< slide->masterCellGroup->getName()
						<< "',";
				out << "GROUP_MA_ESCL='"
						<< slide->slaveCellGroup->getName()
						<< "',";
                out << "COULOMB=" << slide->getFriction() << ",";
				out << ")," << endl;
		}
		for (shared_ptr<Constraint> constraint : surfaces) {
		    shared_ptr<const SurfaceContact> surface = dynamic_pointer_cast<const SurfaceContact>(constraint);
                out << "                             _F(";
				out << "GROUP_MA_MAIT='"
						<< surface->masterCellGroup->getName()
						<< "',";
				out << "GROUP_MA_ESCL='"
						<< surface->slaveCellGroup->getName()
						<< "',";
				out << ")," << endl;
		}
		for (shared_ptr<Constraint> constraint : zones) {
		    shared_ptr<const ZoneContact> zone = dynamic_pointer_cast<const ZoneContact>(constraint);
		    shared_ptr<const ContactBody> master = dynamic_pointer_cast<const ContactBody>(asterModel.model.find(zone->master));
		    shared_ptr<const BoundarySurface> masterSurface = dynamic_pointer_cast<const BoundarySurface>(asterModel.model.find(master->boundary));
		    shared_ptr<const ContactBody> slave = dynamic_pointer_cast<const ContactBody>(asterModel.model.find(zone->slave));
		    shared_ptr<const BoundarySurface> slaveSurface = dynamic_pointer_cast<const BoundarySurface>(asterModel.model.find(slave->boundary));
            out << "                             _F(";

            out << "GROUP_MA_MAIT=(";
                for(auto& cellGroup : masterSurface->getCellGroups()) {
                    out << "'" << cellGroup->getName() << "',";
                }
                out << "),";
            out << "GROUP_MA_ESCL=(";
                for(auto& cellGroup : slaveSurface->getCellGroups()) {
                    out << "'" << cellGroup->getName() << "',";
                }
                out << "),";
            out << ")," << endl;
		}
		out << "                             )," << endl;
		out << "                   );" << endl << endl;
	}
}

void AsterWriter::writeSPC(const AsterModel& asterModel, const ConstraintSet& cset,
		ostream&out) {
  UNUSEDV(asterModel);
	const set<shared_ptr<Constraint>> spcs = cset.getConstraintsByType(Constraint::Type::SPC);
	if (spcs.size() > 0) {
		out << "                   DDL_IMPO=(" << endl;
		for (shared_ptr<Constraint> constraint : spcs) {
			shared_ptr<const SinglePointConstraint> spc = dynamic_pointer_cast<
					const SinglePointConstraint>(constraint);
			//FIXME: filter spcs with type function.
			if (spc->hasReferences()) {
				cerr << "SPC references not supported " << *spc << endl;
				out << " ************************" << endl << "SPC references not supported "
						<< *spc
						<< endl;
			} else {
				out << "                             _F(";
				if (spc->group == nullptr) {
					out << "NOEUD=(";
					for (int nodePosition : spc->nodePositions()) {
						out << "'"
								<< Node::MedName(nodePosition)
								<< "', ";
					}
					out << "),";
				} else {
					out << "GROUP_NO='" << spc->group->getName() << "',";
				}
				//parameter 0 ignored
				for (const DOF dof : spc->getDOFSForNode(0)) {
					if (dof == DOF::DX)
						out << "DX";
					if (dof == DOF::DY)
						out << "DY";
					if (dof == DOF::DZ)
						out << "DZ";
					if (dof == DOF::RX)
						out << "DRX";
					if (dof == DOF::RY)
						out << "DRY";
					if (dof == DOF::RZ)
						out << "DRZ";
					out << "=" << spc->getDoubleForDOF(dof) << ", ";
				}
				out << ")," << endl;
			}
		}
		out << "                             )," << endl;
	}
}
void AsterWriter::writeLIAISON_SOLIDE(const AsterModel& asterModel, const ConstraintSet& cset,
		ostream& out) {
  UNUSEDV(asterModel);

	const set<shared_ptr<Constraint>> rigidConstraints = cset.getConstraintsByType(
			Constraint::Type::RIGID);
	const set<shared_ptr<Constraint>> quasiRigidConstraints = cset.getConstraintsByType(
			Constraint::Type::QUASI_RIGID);
	vector<shared_ptr<Constraint>> constraints;
	constraints.reserve(rigidConstraints.size() + quasiRigidConstraints.size());
	constraints.assign(rigidConstraints.begin(), rigidConstraints.end());
	for (shared_ptr<Constraint> quasiRigidConstraint : quasiRigidConstraints) {
		if ((dynamic_pointer_cast<QuasiRigidConstraint>(quasiRigidConstraint)->isCompletelyRigid())) {
			constraints.push_back(quasiRigidConstraint);
		}
	}
	bool needLiaisonSolide = constraints.size() > 0;

	if (needLiaisonSolide) {
		out << "                   LIAISON_SOLIDE=(" << endl;
		for (auto constraintPtr : constraints) {
			out << "                                   _F(NOEUD=(";
			for (int node : constraintPtr->nodePositions()) {
				out << "'" << Node::MedName(node) << "',";
			}
			out << ")," << endl;
			out << "                                      )," << endl;

		}
		out << "                                   )," << endl;
	}
}

void AsterWriter::writeRBE3(const AsterModel& asterModel, const ConstraintSet& cset,
		ostream& out) {
  UNUSEDV(asterModel);
	const set<shared_ptr<Constraint>> constraints = cset.getConstraintsByType(Constraint::Type::RBE3);
	if (constraints.size() > 0) {
		out << "                   LIAISON_RBE3=(" << endl;
		for (auto constraint : constraints) {
			shared_ptr<const RBE3> rbe3 = dynamic_pointer_cast<const RBE3>(constraint);
			int masterNode = rbe3->getMaster();
			out << "                                 _F(NOEUD_MAIT='"
					<< Node::MedName(masterNode) << "',"
					<< endl;
			out << "                                    DDL_MAIT=(";
			DOFS dofs = rbe3->getDOFSForNode(masterNode);
			if (dofs.contains(DOF::DX))
				out << "'DX',";
			if (dofs.contains(DOF::DY))
				out << "'DY',";
			if (dofs.contains(DOF::DZ))
				out << "'DZ',";
			if (dofs.contains(DOF::RX))
				out << "'DRX',";
			if (dofs.contains(DOF::RY))
				out << "'DRY',";
			if (dofs.contains(DOF::RZ))
				out << "'DRZ',";
			out << ")," << endl;
			set<int> slaveNodes = rbe3->getSlaves();

			out << "                                    NOEUD_ESCL=(";
			for (int slaveNode : slaveNodes) {
				out << "'" << Node::MedName(slaveNode) << "',";
			}
			out << ")," << endl;
			out << "                                    DDL_ESCL=(";
			for (int slaveNode : slaveNodes) {
				DOFS slaveDofs = rbe3->getDOFSForNode(slaveNode);
				int size = 0;
				out << "'";
				if (slaveDofs.contains(DOF::DX)) {
					out << "DX";
					if (++size < slaveDofs.size())
						out << "-";
				}
				if (slaveDofs.contains(DOF::DY)) {
					out << "DY";
					if (++size < slaveDofs.size())
						out << "-";
				}
				if (slaveDofs.contains(DOF::DZ)) {
					out << "DZ";
					if (++size < slaveDofs.size())
						out << "-";
				}
				if (slaveDofs.contains(DOF::RX)) {
					out << "DRX";
					if (++size < slaveDofs.size())
						out << "-";
				}
				if (slaveDofs.contains(DOF::RY)) {
					out << "DRY";
					if (++size < slaveDofs.size())
						out << "-";
				}
				if (slaveDofs.contains(DOF::RZ)) {
					out << "DRZ";
				}
				out << "',";
			}
			out << ")," << endl;
			out << "                                    COEF_ESCL=(";
			for (int slaveNode : slaveNodes) {
				out << rbe3->getCoefForNode(slaveNode) << ",";
			}
			out << ")," << endl;
			out << "                                    )," << endl;
		}
		out << "                                 )," << endl;
	}
}

void AsterWriter::writeLMPC(const AsterModel& asterModel, const ConstraintSet& cset,
		ostream& out) {
  UNUSEDV(asterModel);
	const set<shared_ptr<Constraint>> lmpcs = cset.getConstraintsByType(Constraint::Type::LMPC);
	if (lmpcs.size() > 0) {
		out << "                   LIAISON_DDL=(" << endl;
		for (shared_ptr<Constraint> constraint : lmpcs) {
			shared_ptr<const LinearMultiplePointConstraint> lmpc = dynamic_pointer_cast<
					const LinearMultiplePointConstraint>(constraint);
			out << "                                _F(NOEUD=(";
			set<int> nodes = lmpc->nodePositions();
			for (int nodePosition : nodes) {
				string nodeName = Node::MedName(nodePosition);
				DOFS dofs = lmpc->getDOFSForNode(nodePosition);
				for (int i = 0; i < dofs.size(); i++) {
					out << "'" << nodeName << "', ";
				}
			}
			out << ")," << endl;
			out << "                                   DDL=(";
			for (int nodePosition : nodes) {
				DOFS dofs = lmpc->getDOFSForNode(nodePosition);
				if (dofs.contains(DOF::DX))
					out << "'DX', ";
				if (dofs.contains(DOF::DY))
					out << "'DY', ";
				if (dofs.contains(DOF::DZ))
					out << "'DZ', ";
				if (dofs.contains(DOF::RX))
					out << "'DRX', ";
				if (dofs.contains(DOF::RY))
					out << "'DRY', ";
				if (dofs.contains(DOF::RZ))
					out << "'DRZ', ";
			}
			out << ")," << endl;
			out << "                                   COEF_MULT=(";
			for (int nodePosition : nodes) {
			    DOFCoefs dofcoef = lmpc->getDoFCoefsForNode(nodePosition);
				for (int i = 0; i < 6; i++) {
					if (!is_zero(dofcoef[i]))
						out << dofcoef[i] << ", ";
				}
			}
			out << ")," << endl;
			out << "                                   COEF_IMPO=" << lmpc->coef_impo << "),"
					<< endl;
		}
		out << "                               )," << endl;
	}
}

void AsterWriter::writeGravity(const LoadSet& loadSet, ostream& out) {
	const set<shared_ptr<Loading>> gravities = loadSet.getLoadingsByType(Loading::Type::GRAVITY);
	if (gravities.size() > 0) {
		out << "                      PESANTEUR=(" << endl;
		for (shared_ptr<Loading> loading : gravities) {
			shared_ptr<Gravity> gravity = dynamic_pointer_cast<Gravity>(loading);
			out << "                                 _F(GRAVITE=" << gravity->getAcceleration()
					<< "," << endl;
			VectorialValue direction = gravity->getDirection();
			out << "                                    DIRECTION=(" << direction.x() << ","
					<< direction.y() << "," << direction.z() << "),)," << endl;
		}
		out << "                                 )," << endl;
	}
}

void AsterWriter::writeRotation(const LoadSet& loadSet, ostream& out) {
	const set<shared_ptr<Loading>> rotations = loadSet.getLoadingsByType(Loading::Type::ROTATION);
	if (rotations.size() > 0) {
		out << "                      ROTATION=(" << endl;
		for (shared_ptr<Loading> loading : rotations) {
			shared_ptr<Rotation> rotation = dynamic_pointer_cast<Rotation>(loading);
			out << "                                 _F(VITESSE=" << rotation->getSpeed() << ","
					<< endl;
			VectorialValue axis = rotation->getAxis();
			out << "                                    AXE=(" << axis.x() << "," << axis.y() << ","
					<< axis.z() << ")," << endl;
			VectorialValue center = rotation->getCenter();
			out << "                                    CENTRE=(" << center.x() << "," << center.y()
					<< "," << center.z() << ")";
			out << ",)," << endl;
		}
		out << "                                 )," << endl;
	}
}

void AsterWriter::writeNodalForce(const AsterModel& asterModel, const LoadSet& loadSet, ostream& out) {
  UNUSEDV(asterModel);
	const set<shared_ptr<Loading>> nodalForces = loadSet.getLoadingsByType(Loading::Type::NODAL_FORCE);
	if (nodalForces.size() > 0) {
		out << "                      FORCE_NODALE=(" << endl;
		for (shared_ptr<Loading> loading : nodalForces) {
			shared_ptr<NodalForce> nodal_force = dynamic_pointer_cast<NodalForce>(loading);
			for(auto& nodePosition : nodal_force->nodePositions()) {
                VectorialValue force = nodal_force->getForceInGlobalCS(nodePosition);
                VectorialValue moment = nodal_force->getMomentInGlobalCS(nodePosition);
                out << "                                    _F(NOEUD='"
                        << Node::MedName(nodePosition) << "',";
                if (!is_zero(force.x()))
                    out << "FX=" << force.x() << ",";
                if (!is_zero(force.y()))
                    out << "FY=" << force.y() << ",";
                if (!is_zero(force.z()))
                    out << "FZ=" << force.z() << ",";
                if (!is_zero(moment.x()))
                    out << "MX=" << moment.x() << ",";
                if (!is_zero(moment.y()))
                    out << "MY=" << moment.y() << ",";
                if (!is_zero(moment.z()))
                    out << "MZ=" << moment.z() << ",";
                out << ")," << endl;

			}
		}
		out << "                                    )," << endl;
	}
}

void AsterWriter::writePression(const LoadSet& loadSet, ostream& out) {
	return; // TODO : check if the cellContainer contain skin or shell elements
	const set<shared_ptr<Loading>> loading = loadSet.getLoadingsByType(
			Loading::Type::NORMAL_PRESSION_FACE);
	if (loading.size() > 0) {
		out << "           PRESS_REP=(" << endl;
		for (shared_ptr<Loading> pressionFace : loading) {
			shared_ptr<NormalPressionFace> normalPressionFace = dynamic_pointer_cast<
					NormalPressionFace>(pressionFace);
			out << "                         _F(PRES= " << normalPressionFace->intensity << endl;
			writeCellContainer(*normalPressionFace, out);
			out << "                         )," << endl;
		}
		out << "                      )," << endl;
	}
}

void AsterWriter::writeForceCoque(const LoadSet& loadSet, ostream&out) {
	const set<shared_ptr<Loading> > pressionFaces = loadSet.getLoadingsByType(
			Loading::Type::NORMAL_PRESSION_FACE);
	if (pressionFaces.size() > 0) {
		out << "           FORCE_COQUE=(" << endl;
		for (shared_ptr<Loading> pressionFace : pressionFaces) {
			shared_ptr<NormalPressionFace> normalPressionFace = dynamic_pointer_cast<
					NormalPressionFace>(pressionFace);
			out << "                        _F(PRES=" << normalPressionFace->intensity << ",";
			writeCellContainer(*normalPressionFace, out);
			out << "                         )," << endl;
		}
		out << "            )," << endl;
	}
}

void AsterWriter::writeForceLine(const LoadSet& loadset, ostream& out) {
	const set<shared_ptr<Loading> > forcesLine = loadset.getLoadingsByType(Loading::Type::FORCE_LINE);
	vector<shared_ptr<ForceLine>> forcesOnPoutres;
	vector<shared_ptr<ForceLine>> forcesOnGeometry;

	for (shared_ptr<Loading> loadingForceLine : forcesLine) {
		shared_ptr<ForceLine> forceLine = dynamic_pointer_cast<ForceLine>(loadingForceLine);
		if (forceLine->appliedToGeometry()) {
			forcesOnGeometry.push_back(forceLine);
		} else {
			forcesOnPoutres.push_back(forceLine);
		}
	}
	if (forcesOnPoutres.size() > 0) {
		out << "           FORCE_POUTRE=(" << endl;
		for (shared_ptr<ForceLine> forceLine : forcesOnPoutres) {
            out << "                   _F(";
            switch(forceLine->dof.code) {
            case DOF::Code::DX_CODE:
                out << "FX";
                break;
            case DOF::Code::DY_CODE:
                out << "FY";
                break;
            case DOF::Code::DZ_CODE:
                out << "FZ";
                break;
            case DOF::Code::RX_CODE:
                out << "MX";
                break;
            case DOF::Code::RY_CODE:
                out << "MY";
                break;
            case DOF::Code::RZ_CODE:
                out << "MZ";
                break;
            default:
                throw logic_error("DOF not yet handled");
            }
            out << "=" << asternameByValue[forceLine->force->getReference()] << ",";
            writeCellContainer(*forceLine, out);
            out << "          )," << endl;
		}
		out << "            )," << endl;
	}

	if (forcesOnGeometry.size() > 0) {
		cerr << "ERROR! force_arete not implemented" << endl;
		out << "# warn! force_arete not implemented" << endl;
	}

}
void AsterWriter::writeForceSurface(const LoadSet& loadSet, ostream&out) {
	const set<shared_ptr<Loading> > forceSurfaces = loadSet.getLoadingsByType(
			Loading::Type::FORCE_SURFACE);
	if (forceSurfaces.size() > 0) {
		out << "           FORCE_FACE=(" << endl;
		for (auto& loading : forceSurfaces) {
			shared_ptr<ForceSurface> forceSurface = dynamic_pointer_cast<ForceSurface>(loading);
			VectorialValue force = forceSurface->getForce();
			VectorialValue moment = forceSurface->getMoment();
			out << "                       _F(";
			writeCellContainer(*forceSurface, out);
			out << endl;
			if (!is_equal(force.x(), 0))
				out << "FX=" << force.x() << ",";
			if (!is_equal(force.y(),0))
				out << "FY=" << force.y() << ",";
			if (!is_equal(force.z(),0))
				out << "FZ=" << force.z() << ",";
			if (!is_equal(moment.x(), 0))
				out << "MX=" << moment.x() << ",";
			if (!is_equal(moment.y(), 0))
				out << "MY=" << moment.y() << ",";
			if (!is_equal(moment.z(), 0))
				out << "MZ=" << moment.z() << ",";
			out << "                           )," << endl;
		}
		out << "            )," << endl;
	}
}

void AsterWriter::writeCellContainer(const CellContainer& cellContainer, ostream& out) {
    int celem = 0;
    if (cellContainer.hasCellGroups()) {
      out << "GROUP_MA=(";
      for (auto& cellGroup : cellContainer.getCellGroups()) {
        celem++;
        out << "'" << cellGroup->getName() << "',";
        if (celem % 6 == 0) {
          out << endl << "                             ";
        }
      }
      out << "),";
    }
    if (cellContainer.hasCells()) {
      out << "MAILLE=(";
      for (Cell cell : cellContainer.getCells()) {
        celem++;
        out << "'M" << cell.id << "',";
        if (celem % 6 == 0) {
          out << endl << "                             ";
        }
      }
      out << "),";
    }
}

shared_ptr<NonLinearStrategy> AsterWriter::getNonLinearStrategy(
		NonLinearMecaStat& nonLinAnalysis) {
	shared_ptr<NonLinearStrategy> nonLinearStrategy;
	shared_ptr<vega::Objective> strategy = nonLinAnalysis.model.find(
			nonLinAnalysis.strategy_reference);
	switch (strategy->type) {
	case Objective::Type::NONLINEAR_STRATEGY: {
		nonLinearStrategy = dynamic_pointer_cast<NonLinearStrategy>(strategy);
		break;
	}
	default:
		// Nothing to do
		break;
	}
	return nonLinearStrategy;
}

void AsterWriter::writeAssemblage(const AsterModel& asterModel, Analysis& analysis,
		ostream& out) {
    out << "ASSEMBLAGE(MODELE=MODMECA," << endl;
    if (asterModel.model.materials.size() >= 1) {
        out << "           CHAM_MATER=CHMAT," << endl;
    }
    out << "           CARA_ELEM=CAEL," << endl;
    out << "           CHARGE=(" << endl;
    for (shared_ptr<ConstraintSet> constraintSet : analysis.getConstraintSets()) {
        out << "                   BL" << constraintSet->getId() << "," << endl;
    }
    out << "                   )," << endl;
    out << "           NUME_DDL=CO('NUMDDL" << analysis.getId() << "')," << endl;
    out << "           MATR_ASSE=(_F(OPTION='RIGI_MECA', MATRICE=CO('RIGI"
            << analysis.getId() << "'),)," << endl;
    out << "                      _F(OPTION='MASS_MECA', MATRICE=CO('MASS"
            << analysis.getId() << "'),)," << endl;
    out << "                      _F(OPTION='AMOR_MECA', MATRICE=CO('AMOR"
            << analysis.getId() << "'),)," << endl;
    out << "                      )," << endl;
    out << "           );" << endl << endl;
}

void AsterWriter::writeDynamicExcitation(Analysis& analysis,
		ostream& out) {
    for (shared_ptr<LoadSet> loadSet : analysis.getLoadSets()) {
        for (shared_ptr<Loading> loading : loadSet->getLoadings()) {
            if (loading->type == Loading::Type::DYNAMIC_EXCITATION) {
                DynamicExcitation& dynamicExcitation =
                        dynamic_cast<DynamicExcitation&>(*loading);
                out << "FXL" << analysis.getId() << "_"
                        << dynamicExcitation.getLoadSet()->getId()
                        << "= CALC_VECT_ELEM(OPTION='CHAR_MECA'," << endl;
                out << "                        CHARGE=(" << endl;
                out << "                                CHMEC"
                        << dynamicExcitation.getLoadSet()->getId() << "," << endl;
                for (shared_ptr<ConstraintSet> constraintSet : analysis.getConstraintSets()) {
                    out << "                                BL" << constraintSet->getId() << ","
                            << endl;
                }
                out << "                                )," << endl;
                out << "                        CARA_ELEM=CAEL," << endl;
                out << "                        );" << endl << endl;

                out << "FX" << analysis.getId() << "_"
                        << dynamicExcitation.getLoadSet()->getId()
                        << "= ASSE_VECTEUR(VECT_ELEM=FXL" << analysis.getId() << "_"
                        << dynamicExcitation.getLoadSet()->getId() << "," << endl;
                out << "                    NUME_DDL=NUMDDL" << analysis.getId()
                        << endl;
                out << "                    );" << endl << endl;
            }
        }
    }
}

double AsterWriter::writeAnalysis(const AsterModel& asterModel, Analysis& analysis,
		ostream& out, double debut) {
	if (analysis.isOriginal()) {
		out << "# Analysis original id : " << analysis.getOriginalId() << endl;
	}
	switch (analysis.type) {
	case Analysis::Type::LINEAR_MECA_STAT: {
		LinearMecaStat& linearMecaStat = dynamic_cast<LinearMecaStat&>(analysis);

		out << "RESU" << linearMecaStat.getId() << "=MECA_STATIQUE(MODELE=MODMECA," << endl;
		if (asterModel.model.materials.size() >= 1) {
      out << "                    CHAM_MATER=CHMAT," << endl;
		}
		out << "                    CARA_ELEM=CAEL," << endl;
		out << "                    EXCIT=(" << endl;
		for (shared_ptr<LoadSet> loadSet : linearMecaStat.getLoadSets()) {
			out << "                           _F(CHARGE=" << asternameByLoadSet[loadSet->getReference()] << ")," << endl;
		}
		for (shared_ptr<ConstraintSet> constraintSet : linearMecaStat.getConstraintSets()) {
			//GC: dirty fix for #801, a deeper analysis must be done
			if (constraintSet->getConstraints().size() > 0) {
				out << "                           _F(CHARGE=" << asternameByConstraintSet[constraintSet->getReference()] << "),"
						<< endl;
			}
		}
		out << "                           )," << endl;
                out << "                    SOLVEUR=_F(RENUM='PORD',METHODE='MUMPS')," << endl;
		out << "                    );" << endl << endl;
		break;
	}
	case Analysis::Type::NONLINEAR_MECA_STAT: {
		NonLinearMecaStat& nonLinAnalysis = dynamic_cast<NonLinearMecaStat&>(analysis);
		shared_ptr<NonLinearStrategy> nonLinearStrategy = getNonLinearStrategy(nonLinAnalysis);
		if (!nonLinAnalysis.previousAnalysis) {
			debut = 0;
		}
		double fin = debut + 1.0;
		StepRange stepRange(asterModel.model, debut, nonLinearStrategy->number_of_increments, fin);
		debut = fin;
		string list_name = writeValue(stepRange, out);
		out << "LAUTO" << nonLinAnalysis.getId()
				<< "=DEFI_LIST_INST(METHODE='AUTO', DEFI_LIST=_F(LIST_INST=" << list_name << ",),);"
				<< endl;
		out << "RAMP" << nonLinAnalysis.getId()
				<< "=DEFI_FONCTION(NOM_PARA='INST', PROL_DROITE='LINEAIRE', VALE=("
				<< stepRange.start << ",0.0," << stepRange.end << ",1.0,));" << endl;
		if (nonLinAnalysis.previousAnalysis) {
			out << "IRAMP" << nonLinAnalysis.getId()
					<< "=DEFI_FONCTION(NOM_PARA='INST', PROL_DROITE='LINEAIRE', VALE=("
					<< stepRange.start << ",1.0," << stepRange.end << ",0.0,));" << endl;
		}
		out << "RESU" << nonLinAnalysis.getId() << "=STAT_NON_LINE(MODELE=MODMECA," << endl;
		if (asterModel.model.materials.size() >= 1) {
            out << "                    CHAM_MATER=CHMAT," << endl;
		}
		out << "                    CARA_ELEM=CAEL," << endl;
		out << "                    EXCIT=(" << endl;
		if (nonLinAnalysis.previousAnalysis) {
			for (shared_ptr<LoadSet> loadSet : nonLinAnalysis.previousAnalysis->getLoadSets()) {
				out << "                           _F(CHARGE=" << asternameByLoadSet[loadSet->getReference()]
						<< ",FONC_MULT=IRAMP" << nonLinAnalysis.getId() << ")," << endl;
			}
		}
		for (shared_ptr<LoadSet> loadSet : nonLinAnalysis.getLoadSets()) {
			out << "                           _F(CHARGE=" << asternameByLoadSet[loadSet->getReference()]
					<< ",FONC_MULT=RAMP" << nonLinAnalysis.getId() << ")," << endl;
		}
		for (shared_ptr<ConstraintSet> constraintSet : nonLinAnalysis.getConstraintSets()) {
			if (constraintSet->hasContacts()) {
				continue;
			}
			//GC: dirty fix for #801, a deeper analysis must be done
			if (constraintSet->getConstraints().size() >= 1) {
				out << "                           _F(CHARGE=" << asternameByConstraintSet[constraintSet->getReference()] << "),"
						<< endl;
			}
		}
		out << "                           )," << endl;
		for (shared_ptr<ConstraintSet> constraintSet : asterModel.model.constraintSets) {
			if (constraintSet->getConstraints().size() == 0) {
				// LD filter empty constraintSet
				continue;
			}
			if (not constraintSet->hasContacts()) {
				continue;
			}
			out << "                    CONTACT=" << asternameByConstraintSet[constraintSet->getReference()] << "," << endl;
		}
		double largeDisp = 0;
		out << "                    COMPORTEMENT=(" << endl;
		for (auto elementSet : asterModel.model.elementSets) {
			if (elementSet->material && elementSet->cellGroup != nullptr) {
				out << "                          _F(GROUP_MA='" << elementSet->cellGroup->getName()
						<< "',";
			} else {
				out << "# WARN Skipping material id " << *elementSet << " because no assignment"
						<< endl;
			}
			const shared_ptr<Nature> binature = elementSet->material->findNature(
					Nature::NatureType::NATURE_BILINEAR_ELASTIC);
			const shared_ptr<Nature> nlelas = elementSet->material->findNature(
					Nature::NatureType::NATURE_NONLINEAR_ELASTIC);
			if (binature) {
				out << "RELATION='VMIS_ISOT_LINE',";
			} else if (nlelas) {
				out << "RELATION='ELAS_VMIS_LINE',";
			} else if (!is_equal(largeDisp, 0) && elementSet->isBeam()) {
				out << "RELATION='ELAS_POUTRE_GR',";
				out << "DEFORMATION='GROT_GDEP',";
			} else {
				out << "RELATION='ELAS',";
			}
			out << ")," << endl;

		}
		out << "                           )," << endl;
		out << "                    INCREMENT=_F(LIST_INST=LAUTO" << nonLinAnalysis.getId() << ",),"
				<< endl;
		out << "                    ARCHIVAGE=_F(LIST_INST=" << list_name << ",)," << endl;
		out << "                    NEWTON=_F(REAC_ITER=1,)," << endl;
		if (nonLinAnalysis.previousAnalysis) {
			out << "                    ETAT_INIT=_F(EVOL_NOLI =RESU"
					<< nonLinAnalysis.previousAnalysis->getId() << ")," << endl;
		}
		out << "                    SOLVEUR=_F(RENUM='PORD',METHODE='MUMPS')," << endl;
		out << "                    );" << endl << endl;
		break;
	}
	case Analysis::Type::LINEAR_DYNA_DIRECT_FREQ: {
        writeAssemblage(asterModel, analysis, out);
        writeDynamicExcitation(analysis, out);
        auto structural_damping = asterModel.model.parameters.find(Model::Parameter::STRUCTURAL_DAMPING);
        auto frequency_of_interest_radians = asterModel.model.parameters.find(Model::Parameter::FREQUENCY_OF_INTEREST_RADIANS);
        bool has_structural_damping = structural_damping != asterModel.model.parameters.end() and frequency_of_interest_radians != asterModel.model.parameters.end() and frequency_of_interest_radians->second > 0;
		if (has_structural_damping) {
            out << "AMST" << analysis.getId()
                <<  " = COMB_MATR_ASSE(COMB_R = _F ( MATR_ASSE = RIGI"
                << analysis.getId()
                << ", COEF_R = " << structural_damping->second / frequency_of_interest_radians->second << "))" << endl;
        }
		LinearDynaDirectFreq& linearDirect = dynamic_cast<LinearDynaDirectFreq&>(analysis);
        out << "RESU" << linearDirect.getId() << " = DYNA_VIBRA(" << endl;
        out << "                   TYPE_CALCUL='HARM'," << endl;
        out << "                   BASE_CALCUL='PHYS'," << endl;
		out << "                   MATR_MASS  = MASS" << linearDirect.getId() << ","
				<< endl;
		out << "                   MATR_RIGI  = RIGI" << linearDirect.getId() << ","
				<< endl;
        if (has_structural_damping) {
            out << "                   MATR_AMOR  = AMST" << linearDirect.getId() << ","
				<< endl;
        } else /* LD Maybe should combine these two cases ? */ {
            out << "                   MATR_AMOR  = AMOR" << linearDirect.getId() << ","
				<< endl;
        }
        out << "                   LIST_FREQ  = LST" << setfill('0') << setw(5)
        << linearDirect.getExcitationFrequencies()->getValue()->getId() << "," << endl;
		out << "                   EXCIT      = (" << endl;
		for (shared_ptr<LoadSet> loadSet : linearDirect.getLoadSets()) {
			for (shared_ptr<Loading> loading : loadSet->getLoadings()) {
				if (loading->type == Loading::Type::DYNAMIC_EXCITATION) {
					DynamicExcitation& dynamicExcitation =
							dynamic_cast<DynamicExcitation&>(*loading);
					out << "                                 _F(" << endl;
                    out << "                                    VECT_ASSE=FX"
                            << linearDirect.getId() << "_"
                            << dynamicExcitation.getLoadSet()->getId() << "," << endl;
					out << "                                    FONC_MULT = FCT" << setfill('0')
							<< setw(5) << dynamicExcitation.getFunctionTableB()->getId() << ","
							<< endl;
					out << "                                    PHAS_DEG = "
							<< dynamicExcitation.getDynaPhase()->get() << ",)," << endl;
				}
			}
		}
		out << "                                 )," << endl;
		out << "                   #SOLVEUR=_F(RENUM='PORD',METHODE='MUMPS',NPREC=8)," << endl; // MUMPS: Error in function orderMinPriority no valid number of stages in multisector (#stages = 2)
		out << "                   );" << endl << endl;
	    break;
	}
	case Analysis::Type::LINEAR_MODAL:
	case Analysis::Type::LINEAR_DYNA_MODAL_FREQ: {
        writeAssemblage(asterModel, analysis, out);
        writeDynamicExcitation(analysis, out);

		LinearModal& linearModal = dynamic_cast<LinearModal&>(analysis);

		if (analysis.type == Analysis::Type::LINEAR_MODAL)
			out << "RESU";
		else
			out << "MODES";
		out << linearModal.getId() << "=CALC_MODES(MATR_RIGI=RIGI" << linearModal.getId()
				<< "," << endl;
		out << "                       MATR_MASS=MASS" << linearModal.getId() << "," << endl;
		if (linearModal.use_power_iteration) {
            out << "                       SOLVEUR_MODAL=_F(OPTION_INV='DIRECT')," << endl;
		} else {
            out << "                       SOLVEUR_MODAL=_F(METHODE='TRI_DIAG')," << endl;
		}
		FrequencyTarget& frequencySearch = *(linearModal.getFrequencySearch());
		switch(frequencySearch.frequencyType) {
        case FrequencyTarget::FrequencyType::BAND: {
          BandRange band = dynamic_cast<BandRange&>(*frequencySearch.getValue());
          if (linearModal.use_power_iteration) {
              out << "                       OPTION='SEPARE'," << endl;
          } else if (is_equal(band.end, Globals::UNAVAILABLE_DOUBLE)) {
              out << "                       OPTION='CENTRE'," << endl; // LD : could be replaced by PLUS_PETITE + FILTRE
          } else {
              out << "                       OPTION='BANDE'," << endl;
          }
          out << "                       CALC_FREQ=_F(" << endl;
          out << "                                    FREQ=(";
          if (is_equal(band.start, Globals::UNAVAILABLE_DOUBLE)) {
            auto lower_cutoff_frequency = asterModel.model.parameters.find(Model::Parameter::LOWER_CUTOFF_FREQUENCY);
            if (lower_cutoff_frequency != asterModel.model.parameters.end()) {
                if (asterModel.model.configuration.logLevel >= LogLevel::TRACE) {
                    cout << "Parameter LOWER_CUTOFF_FREQUENCY present, redefining frequency band" << endl;
                }
                out << lower_cutoff_frequency->second;
            } else {
              out << 0.0;
            }
          } else {
            out << band.start;
          }
          if (is_equal(band.end, Globals::UNAVAILABLE_DOUBLE)) {
              out << ", " << ")," << endl;
          } else {
              out << ", " << band.end << ")," << endl;
          }
          if (not is_equal(band.maxsearch, Globals::UNAVAILABLE_DOUBLE)) {
              out << "                                    NMAX_FREQ=" << band.maxsearch << endl;
          }
          out << "                                    )," << endl;
          break;
        }
        case FrequencyTarget::FrequencyType::STEP: {
          StepRange frequencyStep = dynamic_cast<StepRange&>(*frequencySearch.getValue());
          if (linearModal.use_power_iteration) {
            out << "                       OPTION='SEPARE'," << endl; // Not using 'PROCHE' because it will always produce modes, even if not finding them
          } else {
            out << "                       OPTION='CENTRE'," << endl;
          }
          out << "                       CALC_FREQ=_F(" << endl;
          out << "                                    FREQ=(";
          for (double frequency = frequencyStep.start; frequency < frequencyStep.end; frequency += frequencyStep.step ) {
            out << frequency << ",";
          }
          out << ")," << endl;
          out << "                                    )," << endl;
          break;
        }
        case FrequencyTarget::FrequencyType::LIST: {
          ListValue<double>& frequencyList = dynamic_cast<ListValue<double>&>(*frequencySearch.getValue());
          if (linearModal.use_power_iteration) {
            out << "                       OPTION='SEPARE'," << endl; // Not using 'PROCHE' because it will always produce modes, even if not finding them
          } else {
            out << "                       OPTION='CENTRE'," << endl;
          }
          out << "                       CALC_FREQ=_F(" << endl;
          out << "                                    FREQ=(";
          for (const double frequency : frequencyList.getList()) {
              out << frequency << ",";
          }
          out << ")," << endl;
          out << "                                    )," << endl;
          break;
        }
        default:
          handleWritingError(
                    "Frequency search " + to_string(static_cast<int>(frequencySearch.frequencyType)) + " not (yet) implemented");
		}
		out << "                       VERI_MODE=_F(STOP_ERREUR='NON',)," << endl;
		out << "                       SOLVEUR=_F(METHODE='MUMPS'," << endl;
		out << "                                  RENUM='PORD'," << endl;
		out << "                                  NPREC=8," << endl;
		out << "                                  )," << endl;
		out << "                       );" << endl << endl;
		if (analysis.type == Analysis::Type::LINEAR_MODAL)
			break;

		LinearDynaModalFreq& linearDynaModalFreq = dynamic_cast<LinearDynaModalFreq&>(analysis);

		if (linearDynaModalFreq.residual_vector) {
			out << "MOSTA" << linearDynaModalFreq.getId() << "=MODE_STATIQUE(MATR_RIGI=RIGI"
					<< linearDynaModalFreq.getId() << "," << endl;
			out << "                     MATR_MASS=MASS" << linearDynaModalFreq.getId() << ","
					<< endl;
			out << "                     FORCE_NODALE=(" << endl;
			for (shared_ptr<LoadSet> loadSet : linearDynaModalFreq.getLoadSets()) {
				for (auto loading : loadSet->getLoadings()) {
					if (loading->type == Loading::Type::DYNAMIC_EXCITATION) {
						DynamicExcitation& dynamicExcitation =
								dynamic_cast<DynamicExcitation&>(*loading);
						const set<shared_ptr<Loading>> nodalForces =
								dynamicExcitation.getLoadSet()->getLoadingsByType(
										Loading::Type::NODAL_FORCE);
						for (auto loading2 : nodalForces) {
							shared_ptr<NodalForce> nodal_force = dynamic_pointer_cast<NodalForce>(
									loading2);
                            for(auto& nodePosition : nodal_force->nodePositions()) {
                                VectorialValue force = nodal_force->getForceInGlobalCS(nodePosition);
                                VectorialValue moment = nodal_force->getMomentInGlobalCS(nodePosition);
                                out << "                                    _F(NOEUD='"
                                        << Node::MedName(nodePosition) << "'," << endl;
                                out << "                                      AVEC_CMP=(";
                                if (!is_equal(force.x(),0))
                                    out << "'DX',";
                                if (!is_equal(force.y(),0))
                                    out << "'DY',";
                                if (!is_equal(force.z(),0))
                                    out << "'DZ',";
                                if (!is_equal(moment.x(),0))
                                    out << "'DRX',";
                                if (!is_equal(moment.y(),0))
                                    out << "'DRY',";
                                if (!is_equal(moment.z(),0))
                                    out << "'DRZ',";
                                out << "))," << endl;
                            }
						}
					}
				}
			}
			out << "                                   )" << endl;
			out << "                     );" << endl << endl;

			out << "RESVE" << linearDynaModalFreq.getId()
					<< "=DEFI_BASE_MODALE(RITZ =(_F(MODE_MECA=MODES" << linearDynaModalFreq.getId()
					<< ",)," << endl;
			out << "                               _F(MODE_INTF=MOSTA"
					<< linearDynaModalFreq.getId() << ",)," << endl;
			out << "                               )," << endl;
			out << "                        NUME_REF=NUMDDL" << linearDynaModalFreq.getId() << ","
					<< endl;
			out << "                        MATRICE=MASS" << linearDynaModalFreq.getId() << ","
					<< endl;
			out << "                        ORTHO='OUI'," << endl;
			out << "                        );" << endl << endl;

			out << "modes" << linearDynaModalFreq.getId() << "=" << "RESVE"
					<< linearDynaModalFreq.getId() << endl;
		} else {
			out << "modes" << linearDynaModalFreq.getId() << "=" << "MODES"
					<< linearDynaModalFreq.getId() << endl;
		}

        out << "PROJ_BASE(BASE=modes" << linearDynaModalFreq.getId() << "," << endl;
        out << "          MATR_ASSE_GENE=(_F(MATRICE=CO('MASSG" << linearDynaModalFreq.getId()
                << "')," << endl;
        out << "                             MATR_ASSE=MASS" << linearDynaModalFreq.getId() << ",),"
                << endl;
        out << "                          _F(MATRICE=CO('RIGIG" << linearDynaModalFreq.getId()
                << "')," << endl;
        out << "                             MATR_ASSE=RIGI" << linearDynaModalFreq.getId() << ",),"
                << endl;
        out << "                          _F(MATRICE=CO('AMORG" << linearDynaModalFreq.getId()
                << "')," << endl;
        out << "                             MATR_ASSE=AMOR" << linearDynaModalFreq.getId() << ",),"
                << endl;
        out << "                          )," << endl;
        out << "          VECT_ASSE_GENE=(" << endl;
        for (shared_ptr<LoadSet> loadSet : linearDynaModalFreq.getLoadSets()) {
            for (shared_ptr<Loading> loading : loadSet->getLoadings()) {
                if (loading->type == Loading::Type::DYNAMIC_EXCITATION) {
                    DynamicExcitation& dynamicExcitation =
                            dynamic_cast<DynamicExcitation&>(*loading);
                    out << "                          _F(VECTEUR=CO('VG"
                            << linearDynaModalFreq.getId() << "_"
                            << dynamicExcitation.getLoadSet()->getId() << "')," << endl;
                    out << "                             VECT_ASSE=FX"
                            << linearDynaModalFreq.getId() << "_"
                            << dynamicExcitation.getLoadSet()->getId() << ",)," << endl;
                }
            }
        }
        out << "                         )," << endl;
        out << "          );" << endl << endl;

		out << "LIMODE" << linearDynaModalFreq.getId() << "=RECU_TABLE(CO=MODES"
				<< linearDynaModalFreq.getId() << "," << endl;
		out << "                  NOM_PARA = 'FREQ');" << endl << endl;

		out << "pfreq" << linearDynaModalFreq.getId() << "= LIMODE" << linearDynaModalFreq.getId()
				<< ".EXTR_TABLE().values()['FREQ']" << endl;

    if (linearDynaModalFreq.getModalDamping() != nullptr) {
      out << "AMMO_I" << linearDynaModalFreq.getId() << "=CALC_FONC_INTERP(FONCTION = FCT"
          << setfill('0') << setw(5)
          << linearDynaModalFreq.getModalDamping()->getFunctionTable()->getId() << ","
          << endl;
      out << "                         VALE_PARA = pfreq" << linearDynaModalFreq.getId() << endl;
      out << "                         );" << endl << endl;

      out << "AMMO_T" << linearDynaModalFreq.getId()
          << "=CREA_TABLE(FONCTION=_F(FONCTION = AMMO_I" << linearDynaModalFreq.getId()
          << ")," << endl;
      out << "                   );" << endl << endl;

      out << "AMMO" << linearDynaModalFreq.getId() << "=AMMO_T" << linearDynaModalFreq.getId()
          << ".EXTR_TABLE().values()['TOUTRESU']" << endl;
    }

    out << "GENE" << linearDynaModalFreq.getId() << " = DYNA_VIBRA(" << endl;
    out << "                   TYPE_CALCUL='HARM'," << endl;
    out << "                   BASE_CALCUL='GENE'," << endl;
    out << "                   MATR_MASS  = MASSG" << linearDynaModalFreq.getId() << ","
            << endl;
    out << "                   MATR_RIGI  = RIGIG" << linearDynaModalFreq.getId() << ","
            << endl;
    out << "                   MATR_AMOR  = AMORG" << linearDynaModalFreq.getId() << ","
            << endl;
    if (linearDynaModalFreq.getModalDamping() != nullptr) {
        out << "                   AMOR_MODAL = _F(AMOR_REDUIT = AMMO"
            << linearDynaModalFreq.getId() << ",)," << endl;
    }
    out << "                   LIST_FREQ  = LST" << setfill('0') << setw(5)
        << linearDynaModalFreq.getExcitationFrequencies()->getValue()->getId() << "," << endl;
		out << "                   EXCIT      = (" << endl;
		for (shared_ptr<LoadSet> loadSet : linearDynaModalFreq.getLoadSets()) {
			for (shared_ptr<Loading> loading : loadSet->getLoadings()) {
				if (loading->type == Loading::Type::DYNAMIC_EXCITATION) {
					DynamicExcitation& dynamicExcitation =
							dynamic_cast<DynamicExcitation&>(*loading);
					out << "                                 _F(" << endl;
                    out << "                                    VECT_ASSE_GENE = VG"
                            << linearDynaModalFreq.getId() << "_"
                            << dynamicExcitation.getLoadSet()->getId() << "," << endl;
					out << "                                    FONC_MULT = FCT" << setfill('0')
							<< setw(5) << dynamicExcitation.getFunctionTableB()->getId() << ","
							<< endl;
					out << "                                    PHAS_DEG = "
							<< dynamicExcitation.getDynaPhase()->get() << ",)," << endl;
				}
			}
		}
		out << "                                 )," << endl;
		out << "                   #SOLVEUR=_F(RENUM='PORD',METHODE='MUMPS',NPREC=8)," << endl; // MUMPS: Error in function orderMinPriority no valid number of stages in multisector (#stages = 2)
		out << "                   );" << endl << endl;

        out << "RESU" << linearDynaModalFreq.getId() << " = REST_GENE_PHYS(RESU_GENE = GENE"
                << linearDynaModalFreq.getId() << "," << endl;
        out << "                       TOUT_ORDRE = 'OUI'," << endl;
        out << "                       NOM_CHAM = 'DEPL'," << endl;
        out << "                       );" << endl << endl;

		break;
	}

	default:
		handleWritingError(
				"Analysis " + Analysis::stringByType.at(analysis.type) + " not (yet) implemented");
	}
	return debut;
}

void AsterWriter::writeNodalDisplacementAssertion(const AsterModel& asterModel,
		const Assertion& assertion, ostream& out) const {
  UNUSEDV(asterModel);
	const NodalDisplacementAssertion& nda = dynamic_cast<const NodalDisplacementAssertion&>(assertion);
	bool relativeComparison = abs(nda.value) >= SMALLEST_RELATIVE_COMPARISON;
	out << "                     CRITERE = "
			<< (relativeComparison ? "'RELATIF'," : "'ABSOLU',") << endl;
	out << "                     NOEUD='" << Node::MedName(nda.nodePosition) << "'," << endl;
	out << "                     NOM_CMP    = '" << AsterModel::DofByPosition.at(nda.dof.position) << "'," << endl;
	out << "                     NOM_CHAM   = 'DEPL'," << endl;
	if (!is_equal(nda.instant, -1)) {
		out << "                     INST = " << nda.instant << "," << endl;
	} else {
		out << "                     NUME_ORDRE = 1," << endl;
	}
    out << "                     REFERENCE = 'SOURCE_EXTERNE'," << endl;
    out << "                     PRECISION = " << nda.tolerance << "," << endl;
	out << "                     VALE_REFE = " << nda.value << "," << endl;
	out << "                     VALE_CALC = " << (is_zero(nda.value) ? 1e-10 : nda.value) << "," << endl;
	out << "                     TOLE_MACHINE = (" << nda.tolerance << "," << 1e-5 << ")," << endl;

}

void AsterWriter::writeNodalComplexDisplacementAssertion(const AsterModel& asterModel,
		const Assertion& assertion, ostream& out) const {
  UNUSEDV(asterModel);
	const NodalComplexDisplacementAssertion& nda =
			dynamic_cast<const NodalComplexDisplacementAssertion&>(assertion);
	bool relativeComparison = abs(nda.value) >= SMALLEST_RELATIVE_COMPARISON;
	out << "                     CRITERE = "
			<< (relativeComparison ? "'RELATIF'," : "'ABSOLU',") << endl;
	out << "                     NOEUD='" << Node::MedName(nda.nodePosition) << "'," << endl;
	out << "                     NOM_CMP = '" << AsterModel::DofByPosition.at(nda.dof.position)
			<< "'," << endl;
	out << "                     NOM_CHAM = 'DEPL'," << endl;
	out << "                     FREQ = " << nda.frequency << "," << endl;
	out << "                     VALE_CALC_C = " << nda.value.real() << "+" << nda.value.imag()
			<< "j,";
	out << endl;
	out << "                     TOLE_MACHINE = (" << (relativeComparison ? nda.tolerance : 1e-5) << "," << 1e-5 << ")," << endl;
}

void AsterWriter::writeFrequencyAssertion(const Assertion& assertion, ostream& out) const {
	const FrequencyAssertion& frequencyAssertion = dynamic_cast<const FrequencyAssertion&>(assertion);

	out << "                     CRITERE = "
			<< (!is_equal(frequencyAssertion.value, 0) ? "'RELATIF'," : "'ABSOLU',") << endl;
	out << "                     PARA = 'FREQ'," << endl;
	out << "                     NUME_MODE = " << frequencyAssertion.number << "," << endl;
	out << "                     VALE_CALC = " << frequencyAssertion.value << "," << endl;
	out << "                     TOLE_MACHINE = " << frequencyAssertion.tolerance << "," << endl;

}
}
} //end of namespace vega

