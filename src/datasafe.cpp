#include <datasafe.hpp>

ACTION datasafe::permission(name scope, name user, bool storing, bool reading)
{
    require_auth(scope);
    check(is_account(user), "User account does not exist");
    
    // Init the _permissionlist table
    permissionlist_table _permissionlist(get_self(), scope.value);

    // Find the record from _permissionlist table
    auto itr = _permissionlist.find(user.value);

    // Store the permission
    if(itr == _permissionlist.end())
    {
        // Create an entry. The scope account pays for the RAM
        _permissionlist.emplace(scope, [&](auto& entry) 
        {
            entry.user = user;
            entry.storeData = storing;
            entry.readData = reading;
            
            // Default values on creation
            entry.rPubKey = "";
            entry.rPubKeyUp = "";
        });
    }
    else
    {
        // Modify the entry of this user
        _permissionlist.modify(itr, scope, [&](auto& entry) 
        {
            entry.storeData = storing;
            entry.readData = reading;
        });
    }
}

ACTION datasafe::setreadkey(name scope, name user, string readPublicKey)
{
    // Need the permission of the user or the scope account itself 
    name payer;
    if(has_auth(scope))
    { 
        require_auth(scope);
        payer = scope;
    }
    else
    {
        require_auth(user);
        payer = user;

        // Check the public key if it is a proposal by the user
        check(readPublicKey.length() > 0, "The public key is too short to be valid.");
    }

    // Init the _permissionlist table
    permissionlist_table _permissionlist(get_self(), scope.value);

    // Find the record from _permissionlist table
    auto itr = _permissionlist.find(user.value);

    // Check if there is an entry in the permission list for this user
    check(itr != _permissionlist.end(), "There is no entry for this scope.");

    // Modify the entry of this user
    _permissionlist.modify(itr, payer, [&](auto& entry) 
    {
    
        check(!(entry.rPubKey.length() > 0 && entry.rPubKey == readPublicKey), "This key is already setted.");
        
        if(has_auth(scope))
        {
            // The scope account set the new public key 
            if(readPublicKey.length() > 0)
            {
                // Set the new public key 
                entry.rPubKey = readPublicKey;
                if(entry.rPubKey == entry.rPubKeyUp)
                {
                    entry.rPubKeyUp = "";
                }
            }
            else
            {
                // if the scope account set no new public key, the proposal of the user will be used
                check(entry.rPubKeyUp.length() > 0, "There is no public key to set the read permission");
                entry.rPubKey = entry.rPubKeyUp;
                entry.rPubKeyUp = "";
            }
        }
        else
        {
            // Set a proposal for his public key
            check(entry.rPubKeyUp != readPublicKey, "This key is already setted as proposal.");
            entry.rPubKeyUp = readPublicKey;
        }
    });
}

ACTION datasafe::removeuser(name scope, name user)
{
    // Need the permission of the scope account
    require_auth(scope);

    // Init the _permissionlist table
    permissionlist_table _permissionlist(get_self(), scope.value);

    // Find the entry from _permissionlist table
    auto itr = _permissionlist.find(user.value);

    check(itr != _permissionlist.end(), "This user does not exist in the permission list");

    // Delete this entry 
    _permissionlist.erase(itr);
}

ACTION datasafe::update(name scope, name user, string& data, uint64_t refBlock, string& refTrx, uint64_t oldRefBlock, string& oldRefTrx)
{
    // Only a user of the permission list have the right to update in the scope
    require_auth(user);
    check(hasStorePermission(scope, user), "The user have no permission to update a block of this scope");

    // Check for input failures
    check(data.length() > 0, "Missing data");
    
    // Init the _datalogs table. Scope of the table is the scope account
    datalogs_table _datalogs(get_self(), scope.value);

    // Find the record from _datalogs table
    auto log_itr = _datalogs.find(user.value);
    if (log_itr == _datalogs.end()) 
    {
        // Create a datalog record if it does not exist
        _datalogs.emplace(user, [&](auto& dlog) 
        {
            dlog.user = user;
            dlog.data = data;
            dlog.refBlock = refBlock;
            dlog.refTrx = refTrx;
        });
    } 
    else 
    {
        // Modify a datalog record if it exists
        _datalogs.modify(log_itr, user, [&](auto& dlog) 
        {
            // Check for failures with the recorded data
            check(oldRefBlock == dlog.refBlock, "The old referring block number doesn`t match");
            check(oldRefTrx == dlog.refTrx, "The old referring transaction doesn`t match");
            check(refBlock >= dlog.refBlock, "The new referring block number is lower than the last one");

            // Set params
            dlog.data = data;
            dlog.refTrx = refTrx;
            dlog.refBlock = refBlock;
        });
    }
}

ACTION datasafe::clear(name scope) 
{
    require_auth(get_self());

    // Init the _datalogs table. Scope of the table is the scope account
    datalogs_table _datalogs(get_self(), scope.value);

    // Delete all records in _datalogs table
    auto log_itr = _datalogs.begin();
    while (log_itr != _datalogs.end()) 
    {
        log_itr = _datalogs.erase(log_itr);
    }
}

ACTION datasafe::clearlog(name scope, name user) 
{
    require_auth(get_self());

    // Init the _datalogs table. Scope of the table is the scope account
    datalogs_table _datalogs(get_self(), scope.value);

    // Find the record from _datalogs table
    auto log_itr = _datalogs.find(user.value);
    
    check(log_itr != _datalogs.end(), "This user does not exist in the log table");

    // Delete this entry 
    _datalogs.erase(log_itr);
}
bool datasafe::hasStorePermission(name scope, name user)
{
    // Init the _permissionlist table
    permissionlist_table _permissionlist(get_self(), scope.value);

    // Find the entry from _permissionlist table
    auto itr = _permissionlist.find(user.value);

    // Return if the user has the permission to store a new block
    if(itr != _permissionlist.end())
    {
        return itr->storeData;
    }

    return false;
}

uint64_t datasafe::getLastRefBlockOrZero(name scope, name user)
{
    datalogs_table _datalogs(get_self(), scope.value);

    // Find the record from _datalogs table
    auto log_itr = _datalogs.find(user.value);
    if (log_itr == _datalogs.end()) 
    {
        return 0;
    }
    else
    {
        return log_itr->refBlock;
    }
}

EOSIO_DISPATCH(datasafe, (update)(clear)(permission)(removeuser)(setreadkey)(clearlog))
