#include <chainbase/chainbase.hpp>

#include <memory>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

enum tables {
  kv_table
};

// Defines a chainbase table structure.
struct key_value : public chainbase::object<kv_table, key_value> {
  CHAINBASE_DEFAULT_CONSTRUCTOR(key_value);

  id_type id;
  std::string key;
  std::string value;
};

struct by_id;
struct by_key;

// Chainbase is built upon Boost.MultiIndex.
using kv_index = boost::multi_index_container<key_value, 
  boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique< boost::multi_index::tag<by_id>, boost::multi_index::member<key_value, key_value::id_type, &key_value::id> >,
    boost::multi_index::ordered_unique< boost::multi_index::tag<by_key>, boost::multi_index::member<key_value, std::string, &key_value::key> >
  >,
  chainbase::allocator<key_value>>;

CHAINBASE_SET_INDEX_TYPE( key_value, kv_index )

int main(int argc, char *argv[]) { 
  // Open a read/write database that has a capacity of 128 MBs
  auto db = chainbase::database{"./chainbase", chainbase::database::read_write, 128 * 1048576};
  // Create the kv_table
  db.add_index<kv_index>();

  const auto& kv_indices = db.get_index<kv_index>().indices();

  // We can create a session instance, although it is not required.
  {
    auto session = db.start_undo_session(true);

    // Inserts a value into the table.
    db.create<key_value>([&](auto& kv) {
      kv.key = "foo1";
      kv.value = "hello world";
    });

    db.create<key_value>([&](auto& kv){
      kv.key = "foo2";
      kv.value = "hello again";
    });

    session.push();
  }

  for (const auto& kv : kv_indices) {
    std::cout << "{key, value} = {" << kv.key << ", " << kv.value << "}" << std::endl;
  }

  {
    auto session = db.start_undo_session(true);

    auto it = kv_indices.get<by_key>().lower_bound("foo1");
    if (it == std::end(kv_indices.get<by_key>())) {
      std::cout << "ERROR: foo1 key not found!" << std::endl;
    } else {
      // Modifies a row in the table.
      db.modify(*it, [&](auto& kv) {
        kv.value = "goodbye";
      });
    }

    it = kv_indices.get<by_key>().lower_bound("foo2");    
    if (it == std::end(kv_indices.get<by_key>())) {
      std::cout << "ERROR: foo2 key not found!" << std::endl;
    } else {
      // Erases a row in the table.
      db.remove(*it);
    }
    
    session.push();
  }

  for (const auto& kv : kv_indices) {
    std::cout << "{key, value} = {" << kv.key << ", " << kv.value << "}" << std::endl;
  }

  return 0; 
}