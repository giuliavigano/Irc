#ifndef REPLIES_HPP
# define REPLIES_HPP

//Success replies
# define RPL_WELCOME			"001"
# define RPL_ENDOFWHO   		"315"
# define RPL_CHANNELMODEIS		"324"
# define RPL_NOTOPIC			"331"
# define RPL_TOPIC				"332"
# define RPL_TOPICWHOTIME		"333"
# define RPL_INVITING			"341"
# define RPL_WHOREPLY   		"352"
# define RPL_NAMREPLY           "353"
# define RPL_ENDOFNAMES         "366"

//Channel related
# define ERR_NOSUCHNICK			"401"
# define ERR_NOSUCHCHANNEL		"403"
# define ERR_CANNOTSENDTOCHAN	"404"
# define ERR_USERNOTINCHANNEL	"441"
# define ERR_NOTONCHANNEL		"442"
# define ERR_USERONCHANNEL		"443"
# define ERR_CHANNELISFULL		"471"
# define ERR_INVITEONLYCHAN		"473"
# define ERR_BADCHANNELKEY		"475"

//Nickname errors
# define ERR_NONICKNAMEGIVEN	"431"
# define ERR_ERRONEUSNICKNAME	"432"
# define ERR_NICKNAMEINUSE		"433"

# define ERR_NOTREGISTERED		"451"

//Common errors
# define ERR_NEEDMOREPARAMS		"461"
# define ERR_ALREADYREGISTERED	"462"
# define ERR_PASSWDMISMATCH		"464"
# define ERR_UNKNOWNCOMMAND		"421"

# define ERR_CHANOPRIVSNEEDED	"482"

#endif