#ifndef _CLASSIFIERRULEDH_
#define _CLASSIFIERRULEDH_

#include <cstdint>
#include <boost/uuid/uuid.hpp>

namespace DNFC
{
class ClassifierRule<typedef D>
{

 public:
   unsigned int getId()
   {
      return this->id;
   }

   void setId(const unsigned int newId)
   {
      this->id = newId;
   }

   bool addField(const unsigned int offset, const std::string &value)
   {
      return this->addField(offset, value, value);
   }

   bool addField(const unsigned int offset, const std::string &value, const std::string &mask)
   {
      for (auto f : this->fields)
      {
         if (f.offset == offset &&
             f.mask == mask &&
             f.valus == value &&)
            return false;
      }
      this->fields.push_back(Field(idFieldCount++, offset, value, mask));
      return true;
   }

   ClassifierRule(const D &payload) : uuid(boost::uuids::random_generator()()),
                                      data(payload),
                                      fields(new std::vector<Field*>){};
   ~ClassifierRule(){};

 private:
   struct Field
   {
      unsigned int uuid;
      unsigned int offset;
      std::string mask;
      std::string value;
   };

   unsigned int uuid;
   unsigned int idFieldCount;
   std::vector<Field> fields;
   D *data;
}
} // namespace DNFC

#endif
