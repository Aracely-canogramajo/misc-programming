// $Id: listmap.tcc,v 1.15 2019-10-30 12:44:53-07 - - $

#include "listmap.h"
#include "debug.h"

//
/////////////////////////////////////////////////////////////////
// Operations on listmap.
/////////////////////////////////////////////////////////////////
//

//
// listmap::~listmap()
//
template <typename key_t, typename mapped_t, class less_t>
listmap<key_t,mapped_t,less_t>::~listmap() {
   DEBUGF ('l', reinterpret_cast<const void*> (this));
   iterator i = begin();
   while(i != end()){
     erase(i);
     i = begin(); //loop
   }
}

//
// iterator listmap::insert (const value_type&)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::insert (const value_type& pair) {
   DEBUGF ('l', &pair << "->" << pair);
   iterator b = begin(); //get start of listmap
   if(b == end()){ //empty listmap]
     node* newnode = new node(anchor(), anchor(), pair); //create new node
     anchor()->next = newnode; //newnode is now begin()
     anchor()->prev = newnode;
     iterator newit(newnode);
     return newit;
   }else{    
     if(find(pair.first) == end()){ //doesnt exist
       while(b != end() and less((*b).first, pair.first)){
         b = b.where->next; //loop until it stops
       }
       if(b == end()){ //reached the end
         node* newnode = new node(anchor(), anchor()->prev, pair);
         anchor()->prev->next = newnode;
         anchor()->prev = newnode;
         iterator newit(newnode);
         return newit;
       }else{
         node* newnode = new node(b.where, b.where->prev, pair); 
         b.where->prev->next = newnode; 
         b.where->prev = newnode; 
         iterator newit(newnode);
         return newit;
       }
     }else{ //it exits
       iterator f = find(pair.first);
       (*f).second = pair.second; //override
       return f;
     }
   }
   
   return iterator();
}

//
// listmap::find(const key_type&)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::find (const key_type& that) {
   DEBUGF ('l', that);
   iterator f = begin();
   while(f != end()){ 
     if(not less((*f).first,that) and not less(that,(*f).first)){
       return f;
     }
     f = f.where->next; //inc
   }
   return end(); //wasnt there
}

//
// iterator listmap::erase (iterator position)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::erase (iterator position) {
   DEBUGF ('l', &*position); 
   iterator temp = position.where->next;

   position.where->next->prev = position.where->prev;
   position.where->prev->next = position.where->next;   
   position.where->prev = nullptr;
   position.where->next = nullptr;
   delete(position.where);

   return temp;
}


// Helper function for =value
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::helper (const value_type& pair) {
  iterator h = begin();
  while(h != end()){
    if(not less((*h).second,pair.second) && not less(pair.second,(*h).second)){
      cout << (*h).first << " = " << (*h).second << endl;
    }
    h = h.where->next;
  }
  return iterator();
}


