/**
 * @file    Routeur.cpp
 * @author  Mickael LE DENMAT
 * @author  Gabriel Dos Santos
 * @brief   Vous trouverez ici toutes les fonctions implementees
 * pour la classe Routeur
 * @date    2021-05-21
 */

#include "../ReseauGraphe.hpp"
#include <cstdlib>

uint8_t Routeur::m_NbRouteur = 0;

/**
 * @brief Constructeur de la classe Routeur.
 */
Routeur::Routeur() : Machine() {
    m_NbRouteur++;
    m_IdRouteur = m_NbRouteur;

    m_Nom = "Routeur" + std::to_string(m_IdRouteur);

    m_TableRoutage.clear();
    m_TableLSADemandes.clear();
    m_TableLSAEnvoyes.clear();
}

/**
 * @brief Destructeur de la classe Routeur.
 */
Routeur::~Routeur() {}

/**
 * @brief Accesseur du nombre de routeur.
 *
 * @return uint8_t le nombre de routeur.
 */
uint8_t Routeur::getNbRouteur() {
    return m_NbRouteur;
}

/**
 * @brief Accesseur de l'identifiant du routeur.
 *
 * @return uint8_t l'identifiant du routeur.
 */
uint8_t Routeur::getIdRouteur() {
    return m_IdRouteur;
}

/**
 * @brief Envoie les trames de la file d'attente à la machine voisine.
 *
 * @param cwnd le nombre de trame a envoyer.
 * @param estAck indique la trame est un accuse de reception ou non.
 */
void Routeur::envoyer(const uint32_t cwnd, const bool estAck) {
    std::cout << m_Nom << " : Debut envoie\n";

    // Utilise pour le retour.
    if (estAck) {
        std::cout << m_Nom << " : Retour\n";

        // L'accuse de reception est la derniere valeur ajoute.
        std::stack<std::bitset<16>> donneeRecu = m_FileDonnees.back();

        // Creation des couches pour desencapsulation.
        Physique couchePhy;
        Internet coucheInt;
        Transport coucheTrans;

        // Desencapsulation.
        std::stack<std::bitset<16>> paquet = couchePhy.desencapsuler(donneeRecu);
        std::stack<std::bitset<16>> segment = coucheInt.desencapsuler(paquet);
        std::bitset<16> donnee = coucheTrans.desencapsuler(segment);

        // Encapsulation
        segment = coucheTrans.encapsuler(donnee);
        paquet = coucheInt.encapsuler(segment);
        donneeRecu = couchePhy.encapsuler(paquet);

        // Trouve le voisin.
        Machine* voisine = getVoisin(trouverMacDest(coucheInt.getIpSrc()));

        // Ajout de trame dans la file de donnee de la machine voisine.
        voisine->setDonnee(donneeRecu);
        voisine->recevoir(cwnd, true);

        std::cout << m_Nom << " : Fin envoie\n";
        return;
    } else {
        std::cout << m_Nom << " : Aller\n";

        // Creation des couches pour desencapsulation.
        Physique couchePhy;
        Internet coucheInt;
        Transport coucheTrans;

        // Vide la file de donnees.
        std::stack<std::bitset<16>> donneeRecu = suppDonnee();

        // Desencapsulation.
        std::stack<std::bitset<16>> paquet = couchePhy.desencapsuler(donneeRecu);
        std::stack<std::bitset<16>> segment = coucheInt.desencapsuler(paquet);
        std::bitset<16> donnee = coucheTrans.desencapsuler(segment);

        // Encapsulation
        segment = coucheTrans.encapsuler(donnee);
        paquet = coucheInt.encapsuler(segment);
        donneeRecu = couchePhy.encapsuler(paquet);

        // Trouve le voisin.
        Machine* voisine = getVoisin(trouverMacDest(coucheInt.getIpDest()));

        // Traitement de la donnee.
        traitement(donneeRecu, voisine->getMac());

        // Ajout de trame dans la file de donnee de la machine voisine.
        voisine->setDonnee(donneeRecu);

        // Envoie des cwnd trames.
        for (int i = 1; i < int(cwnd); ++i) {
            // Vide la file de donnees.
            std::stack<std::bitset<16>> donneeRecu = suppDonnee();

            // Desencapsulation.
            std::stack<std::bitset<16>> paquet = couchePhy.desencapsuler(donneeRecu);
            std::stack<std::bitset<16>> segment = coucheInt.desencapsuler(paquet);
            std::bitset<16> donnee = coucheTrans.desencapsuler(segment);

            // Encapsulation
            segment = coucheTrans.encapsuler(donnee);
            paquet = coucheInt.encapsuler(segment);
            donneeRecu = couchePhy.encapsuler(paquet);

            // Traitement de la trame.
            traitement(donneeRecu, voisine->getMac());

            // Ajout de trame dans la file de donnee de la machine voisine.
            voisine->setDonnee(donneeRecu);
        }

        //
        voisine->recevoir(cwnd, false);
        std::cout << m_Nom << " : Fin envoie\n";
    }
}

/**
 * @brief Recois la trame.
 *
 * @param cwnd Le nombre de trame recu.
 * @param estAck La trame recu est un accuse de reception ou non.
 */
void Routeur::recevoir(const uint32_t cwnd, const bool estAck) {
    std::cout << m_Nom << " : Debut recevoir\n";
    envoyer(cwnd, estAck);
    std::cout << m_Nom << " : Fin recevoir\n";
}

/**
 * @brief Renvoie l'adresse MAC de la machine correspondante a l'adresse IP.
 *
 * @param ip de la machine qui nous interesse.
 * @return MAC correspondante.
 */
MAC Routeur::trouverMacDest(const IPv4 ip) {
    // std::cout << ip << std::endl;
    //
    Machine* m = ReseauGraphe::getMachine(ip);
    // std::cout << *m << std::endl;

    // Le routeur est il dans le meme sous reseau que l'ip ?
    for (IPv4 sousReseauRouteur : m_SousReseau) {
        for (IPv4 sousReseauDest : m->getSousReseaux()) {
            if (sousReseauRouteur == sousReseauDest) {
                return m->getMac();
            }
        }
    }

    // Trouver le chemin pour aller au routeur dans le meme sous reseau que l'ip dest.
    for (auto iter : m_TableRoutage) {
        auto tabLiaison = iter.second;
        uint16_t routeurArrive = tabLiaison[tabLiaison.size() - 1]->m_NumMachine2;
        Routeur* r = ReseauGraphe::getRouteur(uint8_t(routeurArrive));
        std::cout << *r << std::endl;

        // Renvoie du routeur voisin.
        for (IPv4 sousRes : r->getSousReseaux()) {
            if (sousRes == ReseauGraphe::getSousReseau(ip)) {
                return r->getMac();
            }
        }
    }

    //
    std::cout << "ERREUR : Dans le fichier 'Routeur.cpp. ";
    std::cout << "Dans la fonction 'trouverMacDest'. ";
    std::cout << "Aucune adresse MAC trouvee\n";
    exit(EXIT_FAILURE);
}

// TODO: Send to correct neighboor
void Routeur::envoyerOSPF(Routeur* dest, PaquetOSPF* ospf) {
    dest->recevoirOSPF(ospf);
}

void Routeur::recevoirOSPF(PaquetOSPF* ospf) {
    m_FilePaquetsOSPF.emplace(ospf);
    traitementPaquetOSPF();
}

void Routeur::traitementPaquetOSPF() {
    // Recuperation du paquet en debut de file.
    PaquetOSPF* paquet = m_FilePaquetsOSPF.front();

    // Appel a la methode adequate en fonction du type de paquet.
    switch (paquet->getType()) {
        case 1:
            traitementPaquetHello(dynamic_cast<PaquetHello*>(paquet));
            break;

        case 2:
            traitementPaquetDBD(dynamic_cast<PaquetDBD*>(paquet));
            break;

        case 3:
            traitementPaquetLSR(dynamic_cast<PaquetLSR*>(paquet));
            break;

        case 4:
            traitementPaquetLSU(dynamic_cast<PaquetLSU*>(paquet));
            break;

        case 5:
            traitementPaquetLSAck(dynamic_cast<PaquetLSAck*>(paquet));
            break;

        default:
            std::cout << "ERREUR : fichier `Routeur.cpp`\n"
                << "\tmethode `traitementPaquetOSPF`: "
                << "Type de PaquetOSPF inconnu"
                << std::endl;
            exit(EXIT_FAILURE);
    }
}

// Methodes privees
void Routeur::traitementPaquetHello(PaquetHello* hello) {
    // L'identifiant du voisin ne correspond pas avec l'identifiant du routeur courant.
    if (hello->getIdDestinataire() != m_IdRouteur) {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetHello` : "
            << "Identifiant du routeur (#" << m_IdRouteur
            << ") ne correspond pas avec le routeur destinataire du paquet (#"
            << hello->getIdDestinataire() << ")" << std::endl;

        delete hello;
        exit(EXIT_FAILURE);
    }

    Routeur* destinataire = ReseauGraphe::getRouteur(hello->getIdRouteur());
    if (destinataire) {
        std::vector<LSA> listeLSAs;

        // Initialisions de la liste des annonces LSA.
        for (auto iter: m_TableRoutage) {
            Routeur* routeur = iter.first;
            LSA lsa(routeur->getIdRouteur(),
                    routeur->getIdRouteur(),
                    routeur->getSousReseaux()
            );
            listeLSAs.emplace_back(lsa);
        }

        // Envoie d'un paquet DBD au routeur emetteur du paquet Hello.
        PaquetDBD* reponse = new PaquetDBD(listeLSAs);
        reponse->setEntete(DBD, m_IdRouteur);
        envoyerOSPF(destinataire, reponse);

        delete hello;
    } else {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetHello` : "
            << "Le routeur destinataire (#" << hello->getIdRouteur() << ") n'existe pas"
            << std::endl;

        delete hello;
        exit(EXIT_FAILURE);
    }
}

void Routeur::traitementPaquetDBD(PaquetDBD* dbd) {
    std::vector<LSA> LSAs = dbd->getLSAs();
    std::vector<std::bitset<32>> idADemander;

    for (LSA lsa: LSAs) {
        bool trouve = false;

        for (auto iter: m_TableRoutage) {
            Routeur* routeur = iter.first;
            if (lsa.getIdRouteur() == routeur->getIdRouteur()) {
                trouve = true;
            }
        }

        if (!trouve) {
            idADemander.emplace_back(lsa.getIdLSA());
        }
    }

    Routeur* destinataire = ReseauGraphe::getRouteur(dbd->getIdRouteur());
    // Le vecteur d'identifiant n'est pas vide, on doit envoyer un paquet LSR.
    if (destinataire != nullptr && !idADemander.empty()) {
        // Ajout des identifiants demandes a la table.
        m_TableLSADemandes.emplace(std::make_pair(destinataire, &idADemander));

        // Envoie d'un paquet LSR au routeur nous envoyant le paquet DBD.
        PaquetLSR* reponse = new PaquetLSR(dbd->getIdRouteur(), idADemander);
        reponse->setEntete(LSR, m_IdRouteur);
        envoyerOSPF(destinataire, reponse);

        delete dbd;
    } else {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetDBD` : "
            << "Le routeur destinataire (#" << dbd->getIdRouteur() << ") n'existe pas"
            << std::endl;

        delete dbd;
        exit(EXIT_FAILURE);
    }
}

void Routeur::traitementPaquetLSR(PaquetLSR* lsr) {
    // L'identifiant du voisin ne correspond pas avec l'identifiant du routeur courant.
    if (lsr->getIdEmetteur() != m_IdRouteur) {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetLSR` : "
            << "Identifiant du routeur (#" << m_IdRouteur
            << ") ne correspond pas avec le routeur emetteur du paquet DBD (#"
            << lsr->getIdEmetteur() << ")" << std::endl;

        delete lsr;
        exit(EXIT_FAILURE);
    }

    std::vector<std::bitset<32>> idDemandes = lsr->getIdLSADemandes();
    std::vector<LSA> LSAsDemandes;

    for (std::bitset<32> id: idDemandes) {
        for (auto iter: m_TableRoutage) {
            Routeur* routeur = iter.first;

            // L'identifiant du routeur correspond au LSA demande, on l'ajoute a la liste.
            if (routeur->getIdRouteur() == (uint8_t)(id.to_ulong())) {
                LSA lsa(routeur->getIdRouteur(),
                        routeur->getIdRouteur(),
                        routeur->getSousReseaux()
                );
                LSAsDemandes.emplace_back(lsa);
            }
        }
    }

    Routeur* destinataire = ReseauGraphe::getRouteur(lsr->getIdRouteur());
    if (destinataire) {
        // Ajout des identifiants des annonces a la table des LSA envoyes
        m_TableLSAEnvoyes.emplace(std::make_pair(destinataire, &lsr->getIdLSADemandes()));

        // Envoie d'un paquet DBD au routeur envoyant le paquet Hello.
        PaquetLSU* reponse = new PaquetLSU(LSAsDemandes);
        reponse->setEntete(LSU, m_IdRouteur);
        envoyerOSPF(destinataire, reponse);

        delete lsr;
    } else {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetLSR` : "
            << "Le routeur destinataire (#" << lsr->getIdRouteur() << ") n'existe pas"
            << std::endl;

        delete lsr;
        exit(EXIT_FAILURE);
    }
}

// TODO : Renvoyer des LSUs lorsqu'on en recoit et que l'on fait des mises a jours.
void Routeur::traitementPaquetLSU(PaquetLSU* lsu) {
/*
    std::vector<LSA> LSAs = lsu->getLSADemandes();
    std::vector<std::bitset<32>> idLSARecus;
    bool misAJour = false;

    for (LSA lsa: LSAs) {
        bool connu = false;
        for (auto iter: m_TableRoutage) {
            Routeur* routeur = iter.first;

            if (routeur->getIdRouteur() == lsa.getIdRouteur()) {
                misAJour = true;
                // TODO: Add router to routing table
                Routeur* routeur = ReseauGraphe::getRouteur(lsa.getIdRouteur());
                std::vector<Liaison> chemin;
                m_TableRoutage.emplace(routeur, chemin);
            }
        }
    }

    for (auto iter: m_TableRoutage) {
        Routeur* routeur = ReseauGraphe::getRouteur(iter.first->getIdRouteur());
        std::vector<Liaison*> plusCourtChemin = ReseauGraphe::routageDynamique(
            m_IdRouteur,
            routeur->getIdRouteur()
        );
        m_TableRoutage.emplace(routeur, plusCourtChemin);
        // TODO: send LSUs to all router neighboors
    }

    for (auto iter: m_TableLSADemandes) {
        if (iter.first->getIdRouteur() == lsu->getIdRouteur() && !iter.second.empty()) {
            // Envoie d'un paquet LSR au routeur envoyant le paquet LSU,
            // dans le cas ou il manque des LSAs.
            auto destinataire = iter.first;
            PaquetLSR* reponse = new PaquetLSR(iter.first->getIdRouteur(), iter.second);
            reponse->setEntete(LSR, m_IdRouteur);
            envoyerOSPF(destinataire, reponse);

            // Sortie immediate de la fonction.
            delete lsu;
            return;
        }
    }

    for (auto iter: m_TableRoutage) {
        auto destinataire = iter.first;

        // Envoie d'un paquet LSAck au routeur envoyant le paquet LSU.
        if (destinataire->getIdRouteur() == lsu->getIdRouteur()) {
            PaquetLSAck* reponse = new PaquetLSAck(idLSARecus);
            reponse->setEntete(LSAck, m_IdRouteur);
            envoyerOSPF(destinataire, reponse);

            // Sortie immediate de la fonction.
            delete lsu;
            return;
        }
    }
*/
}

void Routeur::traitementPaquetLSAck(PaquetLSAck* ack) {
    std::vector<std::bitset<32>> idLSARecus = ack->getIdLSARecus();
    std::vector<LSA> LSAManquants;

    for (std::bitset<32> idLSARecu: idLSARecus) {
        for (auto iter: m_TableLSAEnvoyes) {
            Routeur* routeur = iter.first;
            std::vector<std::bitset<32>> idLSAEnvoyes = iter.second;

            if (routeur->getIdRouteur() == ack->getIdRouteur()) {
                for (size_t idLSAEnvoye = 0; idLSAEnvoye < idLSAEnvoyes.size(); ++idLSAEnvoye) {
                    if (idLSAEnvoyes[idLSAEnvoye] == idLSARecu) {
                        idLSAEnvoyes.erase(idLSAEnvoyes.begin() + idLSAEnvoye);
                    }
                }
            }
        }
    }

    Routeur* destinataire = ReseauGraphe::getRouteur(ack->getIdRouteur());
    auto recherche = m_TableLSAEnvoyes.find(destinataire);

    if (recherche != m_TableLSAEnvoyes.end()) {
        if (!recherche->second.empty()) {
            std::vector<std::bitset<32>> idLSAManquants = recherche->second;
            for (std::bitset<32> idLSAManquant: idLSAManquants) {
                for (auto iter: m_TableRoutage) {
                    Routeur* routeur = iter.first;

                    if (routeur->getIdRouteur() == (uint8_t)(idLSAManquant.to_ulong())) {
                        LSA LSAManquant(routeur->getIdRouteur(),
                                        routeur->getIdRouteur(),
                                        routeur->getSousReseaux()
                        );
                        LSAManquants.emplace_back(LSAManquant);
                    }
                }
            }

            // Envoie d'un paquet LSU au routeur envoyant le paquet LSAck,
            // dans le cas ou il des LSA n'ont pas ete recus.
            PaquetLSU* reponse = new PaquetLSU(LSAManquants);
            reponse->setEntete(LSU, m_IdRouteur);
            envoyerOSPF(destinataire, reponse);

            delete ack;
        } else {
            // Aucun LSA manquants, rien a renvoyer
            delete ack;
        }
    } else {
        std::cout << "ERREUR : fichier `Routeur.cpp`\n"
            << "\tmethode `traitementPaquetLSAck` : "
            << "Le routeur destinataire (#" << ack->getIdRouteur() << ") n'existe pas"
            << std::endl;

        delete ack;
        exit(EXIT_FAILURE);
    }
}
