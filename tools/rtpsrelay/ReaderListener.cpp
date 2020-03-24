#include "ReaderListener.h"

namespace RtpsRelay {

ReaderListener::ReaderListener(AssociationTable& association_table,
                               SpdpHandler& spdp_handler,
                               DomainStatisticsWriter& stats_writer)
  : association_table_(association_table)
  , spdp_handler_(spdp_handler)
  , stats_writer_(stats_writer)
{}

void ReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  ReaderEntryDataReader_var dr = ReaderEntryDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ReaderListener::on_data_available failed to narrow RtpsRelay::ReaderEntryDataReader\n")));
    return;
  }

  ReaderEntrySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ReaderListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      {
        const auto from = guid_to_repoid(data[idx].guid());
        GuidSet to;
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l ReaderListener::on_data_available add global reader %C\n"), guid_to_string(from).c_str()));
        association_table_.insert(data[idx], to);
        spdp_handler_.replay(from, to);
        stats_writer_.total_readers(association_table_.reader_count());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l ReaderListener::on_data_available remove global reader %C\n"), guid_to_string(guid_to_repoid(data[idx].guid())).c_str()));
      association_table_.remove(data[idx]);
      stats_writer_.total_readers(association_table_.reader_count());
      break;
    }
  }
}

}
