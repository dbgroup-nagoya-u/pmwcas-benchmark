
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <utility>

#include <sstream>
#include <exception>

/**
 * @brief Persistent memory list-based queue.
 *
 */
class PmemQueue {

    /*####################################################################################
   * Public utilities
   *##################################################################################*/

    public:
    
        /**
        * @brief Inserts a new element at the end of the queue.
        *
        */
        void push(long long int value)
        {
            auto pool = pmem::obj::pool_by_vptr(this);
            ::pmem::obj::transaction::run(pool, [this, &value] {
                auto n = ::pmem::obj::make_persistent<Node>(value, nullptr);

                if (head == nullptr) {
                    head = tail = n;
                } else {
                    tail->next = n;
                    tail = n;
                }
            });
        }

        /**
        * @brief Removes the first element in the queue.
        *
        */
        long long int pop()
        {
            long long int ret = 0;
            auto pool = pmem::obj::pool_by_vptr(this);
            ::pmem::obj::transaction::run(pool, [this, &ret] {
                if (head == nullptr)
                    throw std::runtime_error("Nothing to pop");

                ret = head->value;
                auto n = head->next;

                ::pmem::obj::delete_persistent<Node>(head);
                head = n;

                if (head == nullptr)
                    tail = nullptr;
            });

            return ret;
        }

        /**
        * @brief Prints the entire contents of the queue.
        *
        */
        std::string show(void) const
        {
            std::stringstream ss;
            for (auto n = head; n != nullptr; n = n->next)
                ss << n->value << "\n";

            return ss.str();
        }
    
    /*####################################################################################
   * Internal member variables
   *##################################################################################*/

    private:
        /**
        * @brief Internal node definition.
        */
        struct Node {

            /*!
            * @brief Constructor.
            *
            */
            Node(long long int val, ::pmem::obj::persistent_ptr<Node> n)
                : next(std::move(n)), value(std::move(val))
            {
            }

            // Pointer to the next node
            ::pmem::obj::persistent_ptr<Node> next;
            // Value held by this node
            ::pmem::obj::p<long long int> value;
        };

        // The head of the queue
        ::pmem::obj::persistent_ptr<Node> head;
        // The tail of the queue
        ::pmem::obj::persistent_ptr<Node> tail;
};


