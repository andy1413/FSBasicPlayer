class WZPlayerViewController: UIViewController {
    public lazy var playerView: FSVideoView = {
        let view = FSVideoView(frame: .zero)
        view.backgroundColor = .black
        view.statusChangeCallback = playerStatusChangeCallback
        view.renderChangeCallback = playerRenderChangeCallback
        view.frameChangeCallback = playerFrameChangeCallback
        return view
    }()
    
    func viewDidLoad() {
        super.viewDidLoad()
        
        contentView.addSubview(playerSuperView)
        playerSuperView.mas_makeConstraints { make in
            make?.centerX.equalTo()(0)
            make?.top.equalTo()(0)
            make?.width.equalTo()(UIScreen.main.bounds.size.width)
            make?.height.equalTo()(UIScreen.main.bounds.size.width/16*9)
        }
        
        playerSuperView.addSubview(playerView)
        playerView.mas_makeConstraints { make in
            make?.edges.equalTo()(playerSuperView)
        }
        
        playerView.playUrl = "http://....mpd" // dash url.
        playerView.startSecond = 0
        playerView.playMode = PlayMode_dash
        playerView.play()
    }
    
    func playerStatusChangeCallback(_ videoStatus: FSVideoStatus) {
        switch videoStatus {
        case FSVideoStatus_completed:
            break
        case FSVideoStatus_error:
            break
        case FSVideoStatus_loading:
            break
        case FSVideoStatus_playing:
            break
        case FSVideoStatus_stoped:
            break
        default:
            break
        }
    }
    
    func playerRenderChangeCallback(_ renderIndex: Int) {

    }
    
    func playerFrameChangeCallback(_ size: CGSize) {
        let maxHeight: CGFloat = playerMaxHeight
        let height = min(size.height / size.width * wpk_screen_w, maxHeight)
        let width = size.width / size.height * height
        
        self.playerSuperView.mas_updateConstraints { make in
            make?.width.equalTo()(width)
            make?.height.equalTo()(height)
        }
    }
}
