/*
 * BIP32 wallet interface
 */

#ifndef WALLET_H
#define WALLET_H

#include <string>

#include <crypto/public/crypto.h>

namespace cd {

    class BIP32Wallet {
        public:
            BIP32Wallet();
            ~BIP32Wallet();

            void init_from_seed(const std::string &mnemonic);
            bool init_from_file(const std::string &filename);

            void serialize(const std::string &filename);

            /**
             * @return true if a wallet store exists and is valid
             * and was loaded successfully.
             */
            bool load();
            bool store();

            /**
             * Sign the raw transaction, and return output string
             * suitable for inclusion in tx_input of bip10.
             */
            std::string sign_raw_tx(const std::string &txin);

            void print();

        private:
            void deserialize(const std::string &filename) {}

            std::string seed;
            HDNode hdnode;

            static const std::string wallet_filename;

    };
};

#endif
