//
// Created by denn on 2019-01-29.
//

#pragma once

#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <functional>
#include <optional>
#include <cerrno>
#include <system_error>

namespace ed25519 {

    /**
    * Common error handling closure.
    * */
    typedef std::function<void(const std::error_code &code)> ErrorHandler;

    /**
     * Default error handler
     */
    static auto default_error_handler = [](const std::error_code &code) {};

    /**
     * Base58 string to/from encoding/decoding
     * */

    namespace size {
        constexpr const size_t hash        = 32;
        constexpr const size_t double_hash = hash*2;

        constexpr const size_t public_key  = hash;
        constexpr const size_t digest      = hash;
        constexpr const size_t seed        = hash;

        constexpr const size_t private_key = double_hash;
        constexpr const size_t signature   = double_hash;
    }

    enum error:int {
        BADFORMAT = 1000,
        UNEXPECTED_SIZE = 1001,
        EMPTY = 1002,
    };

    class error_category: public std::error_category
    {
    public:
        error_category(const std::string &message);
        error_category():mess_(""){};
        const char* name() const noexcept override;
        std::string message(int ev) const override;

    private:
        std::string mess_;
    };

    namespace base58 {

        uint_least32_t crc32(unsigned char *buf, size_t len);

        /**
         * Encode binary data to base58 string
         * @param binary data
         * @return base58 string
         */
        std::string encode(const std::vector<unsigned char>& data);

        /**
         * Decode base58-encoded string to binary data
         * @param str
         * @param data
         * @return false if decoding is failed
         */
        bool decode(const std::string& str, std::vector<unsigned char>& data);

        /**
         * Validate base58-encoded string
         * @param str
         * @return false if decoding is failed
         */
        bool validate(const std::string &str);

        /**
         * Decode base58 string to binary format
         * @param base58
         * @param error
         * @return binary buffer
         */
        template<size_t N>
        bool decode(
                const std::string &base58,
                std::array<unsigned char, N> &data,
                const ErrorHandler &error = default_error_handler){

            std::vector<unsigned char> v;

            if (!decode(base58.c_str(), v))
            {
                std::error_code ec(static_cast<int>(error::BADFORMAT),error_category());
                error(ec);

                return false;
            }

            if (v.size() != N)
            {
                std::stringstream errorMessage;
                errorMessage << "size of decoded vector is not equal to expected size: " << v.size() << " <> " << N;
                std::error_code ec(static_cast<int>(error::UNEXPECTED_SIZE),error_category(errorMessage.str()));
                error(ec);

                return false;
            }

            std::copy_n(v.begin(), N, data.begin());

            return true;
        }

        /**
         * Encode binary data to base58 string
         * @tparam N
         * @param data
         * @return
         */
        template<size_t N>
        std::string encode(
                const std::array<unsigned char, N> &data) {

            // add 4-byte hash check to the end

            std::vector<unsigned char> vch;
            vch.assign(data.begin(), data.end());


            uint_least32_t crc32_ = crc32(&vch[0], vch.size());

            // little endian
            vch.push_back(static_cast<unsigned char>(crc32_ & 0xff));
            vch.push_back(static_cast<unsigned char>((crc32_ >> 8) & 0xff));
            vch.push_back(static_cast<unsigned char>((crc32_ >> 16) & 0xff));
            vch.push_back(static_cast<unsigned char>((crc32_ >> 24) & 0xff));

            return encode(vch);
        }
    }

    /**
     * Common base58 encoding/decoding protocol
     * */
    class Base58 {
    public:

        /**
         * Clear key data
         */
        virtual void clean() = 0;

        /**
         * Encode key data to base58 string
         * @return encoded base58 string
         */
        virtual const std::string encode() const = 0;

        /**
         *
         * Decode string from base58 string
         *
         * @param base58_string - encoded string
         * @param error - handle error if key couldn't ber read from string
         * @return true or false
         */
        virtual bool decode(const std::string &base58_string, const ErrorHandler &error = default_error_handler) = 0;

        /**
         * Validate base58 string without creating object instance
         * @param base58 string
         * @return true if string is base58 encoded
         */
        static bool validate(const std::string &base58);
    };

    /**
     * Base58 binary data structure
     * @tparam N
     */
    template <size_t N>
    class Data: public std::array<unsigned char, N>, public Base58 {
        typedef std::array<unsigned char, N> binary_data;
    public:
        using binary_data::binary_data;

        Data():binary_data() { clean(); }

        /**
         * Clean data memory
         */
        void clean() override { binary_data::fill(0); }

        /**
         * Encode from binary to base58 encoded string
         * @return encoded string
         */
        const std::string encode() const override {
            return base58::encode(*this);
        }

        /**
         * Decode base58 encoded string to binary represenation
         * @param base58 encoded string
         * @param error - error handler
         * @return false if decoding is failed
         */
        bool decode(const std::string &base58, const ErrorHandler &error = default_error_handler) override{
            return base58::decode(base58, *this, error);
        }

        static bool validate(const std::string &check) {
            return  base58::validate(check);
        }
    };

    namespace keys {

        /**
         * Seed data
         */
        class Seed: public Data<size::seed>{

            typedef Data<size::seed> seed_data;

        public:
            using seed_data::seed_data;

            /**
             * Create seed from secret phrase
             * @param phrase
             */
            Seed(const std::string &phrase);

            /**
             * Create random seed
             */
            Seed();
        };

        /**
         * Public key representaion
         */
        class Public: public Data<size::public_key>{};

        /**
         * Private key representation
         */
        class Private: public Data<size::private_key>{};

        /**
         * Pair key representation
         */
        class Pair {

        public:
            /**
             * Get paired public key for the private
             * @return copy of public key
             */
            const Public  &get_public_key() const { return publicKey_; };
            const Private &get_private_key() const { return privateKey_; };

            /**
             * Create random pair
             * @return always exists private key
             */
            static std::optional<Pair> Random();

            /**
            * Create random pair
            * @return always exists private key
            */
            static std::optional<Pair> FromPrivateKey(const std::string &privateKey, const ErrorHandler &error = default_error_handler);

            /**
             * Optional constructor from secret phrase
             * @param phrase
             * @param error
             * @return nullopt or private key
             */
            static std::optional<Pair> WithSecret(const std::string &phrase,
                                                  const ErrorHandler &error = default_error_handler);

            /**
             * Clean pair
             */
            void clean() {
                publicKey_.clean();
                privateKey_.clean();
            }

            ~Pair() {
                clean();
            }

        private:
            Pair();
            Public publicKey_;
            Private privateKey_;
        };
    }

    const std::string StringFormat(const char* format, ...);
}