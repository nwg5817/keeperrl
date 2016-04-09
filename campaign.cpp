#include "stdafx.h"
#include "campaign.h"
#include "view.h"
#include "view_id.h"
#include "model_builder.h"
#include "model.h"
#include "progress_meter.h"
#include "options.h"
#include "name_generator.h"

template <class Archive> 
void Campaign::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, sites, playerPos, worldName, defeated, influencePos, influenceSize);
}

SERIALIZABLE(Campaign);
SERIALIZATION_CONSTRUCTOR_IMPL(Campaign);

const Table<Campaign::SiteInfo>& Campaign::getSites() const {
  return sites;
}

bool Campaign::SiteInfo::canEmbark() const {
  return !dweller && !blocked;
}

bool Campaign::canTravelTo(Vec2 pos) const {
  return (isInInfluence(pos) || playerPos == pos) && !sites[pos].isEmpty();
}

optional<Vec2> Campaign::getPlayerPos() const {
  return playerPos;
}

Campaign::Campaign(Vec2 size) : sites(size, {}), defeated(size, false) {
}

const string& Campaign::getWorldName() const {
  return worldName;
}

void Campaign::clearSite(Vec2 v) {
  sites[v] = SiteInfo{};
  sites[v].viewId = {ViewId::GRASS};
}

typedef Campaign::VillainInfo::Type TribeType;

static vector<Campaign::VillainInfo> getMainVillains() {
  return {
      {ViewId::AVATAR, EnemyId::KNIGHTS, "Knights", TribeType::MAIN},
      {ViewId::ELF_LORD, EnemyId::ELVES, "Elves", TribeType::MAIN},
      {ViewId::DWARF_BARON, EnemyId::DWARVES, "Dwarves", TribeType::MAIN},
      {ViewId::RED_DRAGON, EnemyId::RED_DRAGON, "Red dragon", TribeType::MAIN},
      {ViewId::ELEMENTALIST, EnemyId::ELEMENTALIST, "Elementalist", TribeType::MAIN},
      {ViewId::GREEN_DRAGON, EnemyId::GREEN_DRAGON, "Green dragon", TribeType::MAIN},
      {ViewId::LIZARDLORD, EnemyId::LIZARDMEN, "Lizardmen", TribeType::MAIN},
      {ViewId::SHAMAN, EnemyId::WARRIORS, "Warriors", TribeType::MAIN},
  };
}

static vector<Campaign::VillainInfo> getLesserVillains() {
  return {
      {ViewId::BANDIT, EnemyId::BANDITS, "Bandits", TribeType::LESSER},
      {ViewId::ENT, EnemyId::ENTS, "Tree spirits", TribeType::LESSER},
      {ViewId::DRIAD, EnemyId::DRIADS, "Driads", TribeType::LESSER},
      {ViewId::CYCLOPS, EnemyId::CYCLOPS, "Cyclops", TribeType::LESSER},
      {ViewId::SHELOB, EnemyId::SHELOB, "Giant spider", TribeType::LESSER},
      {ViewId::HYDRA, EnemyId::HYDRA, "Hydra", TribeType::LESSER},
      {ViewId::ANT_QUEEN, EnemyId::ANTS, "Ants", TribeType::LESSER},
  };
}

static vector<Campaign::VillainInfo> getAllies() {
  return {
      {ViewId::UNKNOWN_MONSTER, EnemyId::FRIENDLY_CAVE, "Unknown", TribeType::ALLY},
      {ViewId::UNKNOWN_MONSTER, EnemyId::SOKOBAN, "Unknown", TribeType::ALLY},
      {ViewId::DARK_ELF_LORD, EnemyId::DARK_ELVES, "Dark elves", TribeType::ALLY},
      {ViewId::GNOME_BOSS, EnemyId::GNOMES, "Gnomes", TribeType::ALLY},
      {ViewId::ORC_CAPTAIN, EnemyId::ORC_VILLAGE, "Greenskin village", TribeType::ALLY},
  };
}

bool Campaign::isDefeated(Vec2 pos) const {
  return defeated[pos];
}

void Campaign::setDefeated(Vec2 pos) {
  defeated[pos] = true;
  refreshInfluencePos();
}

bool Campaign::VillainInfo::isEnemy() const {
  return type != ALLY;
}

string Campaign::VillainInfo::getDescription() const {
  switch (type) {
    case ALLY: return "ally";
    case MAIN: return "main villain";
    case LESSER: return "lesser villain";
  }
}

optional<Campaign::VillainInfo> Campaign::SiteInfo::getVillain() const {
  if (dweller)
    if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::PlayerInfo> Campaign::SiteInfo::getPlayer() const {
  if (dweller)
    if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
      return *info;
  return none;
}

optional<Campaign::RetiredSiteInfo> Campaign::SiteInfo::getRetired() const {
  if (dweller)
    if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
      return *info;
  return none;
}
 
bool Campaign::SiteInfo::isEmpty() const {
  return !dweller;
}

optional<string> Campaign::SiteInfo::getDwellerDescription() const {
  if (!dweller)
    return none;
  if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
    return string("This is your home site");
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->name + " (" + info->getDescription() + ")";
  if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
    return info->name + " (hostile)";
  return none;
}

optional<ViewId> Campaign::SiteInfo::getDwellerViewId() const {
  if (!dweller)
    return none;
  if (const PlayerInfo* info = boost::get<PlayerInfo>(&(*dweller)))
    return info->viewId;
  if (const VillainInfo* info = boost::get<VillainInfo>(&(*dweller)))
    return info->viewId;
  if (const RetiredSiteInfo* info = boost::get<RetiredSiteInfo>(&(*dweller)))
    return info->viewId;
  return none;
}

bool Campaign::SiteInfo::isEnemy() const {
  return getRetired() || (getVillain() && getVillain()->isEnemy());
}

void Campaign::SiteInfo::setBlocked() {
  blocked = true;
  viewId.push_back(Random.choose({ViewId::MAP_MOUNTAIN1, ViewId::MAP_MOUNTAIN2, ViewId::MAP_MOUNTAIN3,
        ViewId::CANIF_TREE, ViewId::DECID_TREE}));
}

bool Campaign::isInInfluence(Vec2 pos) const {
  return influencePos.count(pos);
}

void Campaign::refreshInfluencePos() {
  influencePos.clear();
  if (!playerPos)
    return;
  for (double r = 1; r <= max(sites.getWidth(), sites.getHeight()); r += 0.1) {
    for (Vec2 v : sites.getBounds())
      if ((sites[v].getVillain() || sites[v].getRetired()) && v.distD(*playerPos) <= r)
        influencePos.insert(v);
    int numEnemies = 0;
    for (Vec2 v : influencePos)
      if (sites[v].isEnemy() && !defeated[v])
        ++numEnemies;
    if (numEnemies >= influenceSize)
      break;
  }
}

int Campaign::getNumGenVillains() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getVillain())
      ++ret;
  return ret;
}

int Campaign::getNumRetVillains() const {
  int ret = 0;
  for (Vec2 v : sites.getBounds())
    if (sites[v].getRetired())
      ++ret;
  return ret;
}

optional<Campaign> Campaign::prepareCampaign(View* view, Options* options, const vector<RetiredSiteInfo>& retired,
    function<string()> worldNameGen, RandomGen& random) {
  Vec2 size(16, 9);
  int numBlocked = 0.6 * size.x * size.y;
  string worldName;
  while (1) {
    options->setLimits(OptionId::RETIRED_VILLAINS, 0, min<int>(retired.size(), 4)); 
    options->setLimits(OptionId::MAIN_VILLAINS, 0, 9); 
    options->setLimits(OptionId::LESSER_VILLAINS, 0, 8); 
    options->setLimits(OptionId::ALLIES, 0, 6); 
    options->setLimits(OptionId::INFLUENCE_SIZE, 3, 6); 
    options->setDefaultString(OptionId::KEEPER_NAME, NameGenerator::get(NameGeneratorId::FIRST)->getNext());
    int numRetired = options->getIntValue(OptionId::RETIRED_VILLAINS);
    int numMain = options->getIntValue(OptionId::MAIN_VILLAINS);
    int numLesser = options->getIntValue(OptionId::LESSER_VILLAINS);
    int numAllies = options->getIntValue(OptionId::ALLIES);
    vector<VillainInfo> mainVillains;
    while (mainVillains.size() < numMain)
      append(mainVillains, random.permutation(getMainVillains()));
    mainVillains.resize(numMain);
    vector<VillainInfo> lesserVillains;
    while (lesserVillains.size() < numLesser)
      append(lesserVillains, random.permutation(getLesserVillains()));
    lesserVillains.resize(numLesser);
    vector<VillainInfo> allies;
    while (allies.size() < numAllies)
      append(allies, random.permutation(getAllies()));
    allies.resize(numAllies);
    Campaign campaign(size);
    if (!worldName.empty())
      campaign.worldName = worldName;
    else
      campaign.worldName = worldNameGen();
    for (Vec2 v : Rectangle(size))
      campaign.clearSite(v);
    vector<Vec2> freePos;
    for (Vec2 v : Rectangle(size))
      if (campaign.sites[v].canEmbark())
        freePos.push_back(v);
    for (int i : All(mainVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = mainVillains[i];
    }
    for (int i : Range(numRetired)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = retired[i];
    }
    for (int i : All(lesserVillains)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = lesserVillains[i];
    }
    for (int i : All(allies)) {
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].dweller = allies[i];
    }
    for (int i : Range(numBlocked)) {
      if (freePos.size() <= 1)
        break;
      Vec2 pos = random.choose(freePos);
      removeElement(freePos, pos);
      campaign.sites[pos].setBlocked();
    }
    while (1) {
      bool reroll = false;
      campaign.influenceSize = options->getIntValue(OptionId::INFLUENCE_SIZE);
      campaign.refreshInfluencePos();
      CampaignAction action = view->prepareCampaign(campaign, options);
      switch (action.getId()) {
        case CampaignActionId::REROLL_MAP:
            reroll = true; break;
        case CampaignActionId::UPDATE_OPTION:
            switch (action.get<OptionId>()) {
              case OptionId::KEEPER_NAME:
              case OptionId::INFLUENCE_SIZE: break;
              default: reroll = true; break;
            }
            break;
        case CampaignActionId::CANCEL:
            return none;
        case CampaignActionId::CHOOSE_SITE:
            if (campaign.playerPos)
              campaign.clearSite(*campaign.playerPos);
            campaign.playerPos = action.get<Vec2>();
            campaign.sites[*campaign.playerPos].dweller = PlayerInfo{ViewId::KEEPER};
            break;
        case CampaignActionId::CONFIRM:
            return campaign;
      }
      if (reroll)
        break;
    }
  }
}

