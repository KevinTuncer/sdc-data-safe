#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

// Contract and actions have to be in lowercase and maximal 12 signs
CONTRACT datasafe : public contract 
{
public:
    using contract::contract;

    // Set the storing permission of a user 
    ACTION permission(name scope, name user, bool storing, bool reading);

    // Set the public of a user for data encryption
    ACTION setreadkey(name scope, name user, string readPublicKey);

    // Remove an user from the permission list
    ACTION removeuser(name scope, name user);

    // Update the data for the last blocks
    // oldRefBlock and oldRefTrx are necessary and will be checked to trace old records even if wrong new references will be made
    ACTION update(name scope, name user, string& data, uint64_t refBlock, string& refTrx, uint64_t oldRefBlock, string& oldRefTrx);
    
    // Clear the datalogs table. This is only allowed by the contract creator
    ACTION clear(name scope);

    ACTION clearlog(name scope, name user);

private:
    // Return true if the user has the permission to update a new block
    bool hasStorePermission(name scope, name user);

    //- Not in use:
    // Get the last refering block number of the scope. Return 0 if there is no record
    uint64_t getLastRefBlockOrZero(name scope, name user);

public:

// Tables in the RAM of the eosio blockchain:
    // Each entry is for one uploader account name
    TABLE data_log {
        name            user;           // Account name of the uploader
        string        data;             // Data content of a record
        uint64_t    refBlock;           // Reference to the last block number 
        string        refTrx;           // Reference to the transaction id
        auto primary_key() const { return user.value; }
    };
    typedef multi_index<name("datalogs"), data_log> datalogs_table;

    // The primary_key is the user account
    TABLE permission_entry {
        name            user;           // Account name of the user
        bool            storeData;      // Store permission of the user
        bool            readData;       // Readingpermission of the user
        string        rPubKey;          // Public key which should be used for encryption if readData == true
        string        rPubKeyUp;        // Proposal for a public key. Uploaded by the corresponding user
        auto primary_key() const { return user.value; }
    };
    typedef multi_index<name("permissions"), permission_entry> permissionlist_table;
};
