// GDC_TimeSkipSequence.c
// Path in mod: Scripts/Game/GDC/Teleport/GDC_TimeSkipSequence.c

//! Local part of a teleport: fade to black, move the player, fade back. Runs on the machine of the traveller.
//! Uses its own screen rather than SCR_FadeInOutEffect, which is drawn under the HUD and is reset by the map gadget.
class GDC_TimeSkipSequence
{
	protected const ResourceName SCREEN_LAYOUT = "{7C41E0A95D3B2F68}UI/layouts/Teleport/GDC_TimeSkipScreen.layout";
	//! Above the vanilla teleport feedback (1000), a black flash shown by SCR_Global.TeleportPlayer on the same HUD layer.
	protected const int SCREEN_Z_ORDER = 1001;
	protected const float TEXT_FADE_SPEED = 1;
	protected const int SOUND_UPDATE_MS = 50;

	protected vector m_vPosition;
	protected float m_fBlackDuration;
	protected float m_fFadeInDuration;
	protected bool m_bMuteSound;

	protected bool m_bRunning;
	protected bool m_bControlsDisabled;
	protected Widget m_wScreen;
	protected Widget m_wTextPanel;
	protected CharacterControllerComponent m_CharacterController;

	//------------------------------------------------------------------------------------------------
	bool IsRunning()
	{
		return m_bRunning;
	}

	//------------------------------------------------------------------------------------------------
	void Start(vector position, float fadeOutDuration, float blackDuration, float fadeInDuration, string text, ResourceName backgroundImage, float backgroundMaxScreenPercent, bool muteSound)
	{
		m_vPosition = position;
		m_fBlackDuration = blackDuration;
		m_fFadeInDuration = fadeInDuration;
		m_bMuteSound = muteSound;

		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (fadeOutDuration + blackDuration + fadeInDuration <= 0 || !hudManager)
		{
			Teleport();
			return;
		}

		m_wScreen = hudManager.CreateLayout(SCREEN_LAYOUT, EHudLayers.ALWAYS_TOP, SCREEN_Z_ORDER);
		if (!m_wScreen)
		{
			Teleport();
			return;
		}

		m_bRunning = true;

		TextWidget textWidget = TextWidget.Cast(m_wScreen.FindAnyWidget("Text"));
		if (textWidget && !text.IsEmpty())
		{
			textWidget.SetText(text);
			m_wTextPanel = m_wScreen.FindAnyWidget("TextPanel");
		}

		ShowBackground(backgroundImage, backgroundMaxScreenPercent);
		CloseInterfaces();
		SetControlsDisabled(true);

		m_wScreen.SetOpacity(0);
		AnimateScreen(1, fadeOutDuration);

		if (m_bMuteSound)
			GetGame().GetCallqueue().CallLater(UpdateSound, SOUND_UPDATE_MS, true);

		GetGame().GetCallqueue().CallLater(OnScreenBlack, fadeOutDuration * 1000);
	}

	//------------------------------------------------------------------------------------------------
	//! Shows the image undistorted on the black screen, within the given share of the screen and never above its native resolution.
	protected void ShowBackground(ResourceName backgroundImage, float maxScreenPercent)
	{
		ImageWidget background = ImageWidget.Cast(m_wScreen.FindAnyWidget("Background"));
		if (!background)
			return;

		int imageWidth, imageHeight;
		if (!backgroundImage.IsEmpty() && background.LoadImageTexture(0, backgroundImage))
			background.GetImageSize(0, imageWidth, imageHeight);

		if (imageWidth <= 0 || imageHeight <= 0)
		{
			background.SetVisible(false);
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float screenWidth, screenHeight;
		workspace.GetScreenSize(screenWidth, screenHeight);

		// Computed in screen pixels: 1 is the native resolution. Widget sizes are in DPI independent units.
		float scale = Math.Min(screenWidth / imageWidth, screenHeight / imageHeight) * maxScreenPercent / 100;
		scale = Math.Min(scale, 1);
		background.SetSize(workspace.DPIUnscale(imageWidth * scale), workspace.DPIUnscale(imageHeight * scale));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnScreenBlack()
	{
		// Moved at the start of the black screen, so its whole duration is available to load the destination.
		Teleport();

		if (m_wTextPanel)
			AnimateWidget.Opacity(m_wTextPanel, 1, TEXT_FADE_SPEED);

		GetGame().GetCallqueue().CallLater(OnBlackEnd, m_fBlackDuration * 1000);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBlackEnd()
	{
		SetControlsDisabled(false);
		AnimateScreen(0, m_fFadeInDuration);
		GetGame().GetCallqueue().CallLater(Finish, m_fFadeInDuration * 1000);
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish()
	{
		m_bRunning = false;
		CleanUp();
	}

	//------------------------------------------------------------------------------------------------
	protected void Teleport()
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());

		// The situation may have changed since the server decided: a vehicle would be moved with the player.
		if (character && !character.IsInVehicle() && !character.GetCharacterController().IsDead())
			SCR_Global.TeleportLocalPlayer(m_vPosition);
	}

	//------------------------------------------------------------------------------------------------
	//! \param duration In seconds. The speed of a widget animation is the inverse of a duration.
	protected void AnimateScreen(float opacity, float duration)
	{
		if (duration > 0)
			AnimateWidget.Opacity(m_wScreen, opacity, 1 / duration);
		else
			m_wScreen.SetOpacity(opacity);
	}

	//------------------------------------------------------------------------------------------------
	//! The sound follows the picture. Volumes can't be read from script, 1 is the value vanilla cinematics return to.
	protected void UpdateSound()
	{
		if (m_wScreen)
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 1 - m_wScreen.GetOpacity());
	}

	//------------------------------------------------------------------------------------------------
	//! The player must not arrive with the map or the inventory still open.
	protected void CloseInterfaces()
	{
		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		if (character)
		{
			SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(character);
			if (gadgetManager)
				gadgetManager.RemoveHeldGadget();
		}

		GetGame().GetMenuManager().CloseAllMenus();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetControlsDisabled(bool disabled)
	{
		if (disabled)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
			if (character)
				m_CharacterController = character.GetCharacterController();
		}

		// The reference is lost if the character is deleted meanwhile, for instance after a respawn.
		if (m_CharacterController)
		{
			m_CharacterController.SetDisableMovementControls(disabled);
			m_CharacterController.SetDisableWeaponControls(disabled);
		}

		m_bControlsDisabled = disabled;
	}

	//------------------------------------------------------------------------------------------------
	protected void CleanUp()
	{
		GetGame().GetCallqueue().Remove(OnScreenBlack);
		GetGame().GetCallqueue().Remove(OnBlackEnd);
		GetGame().GetCallqueue().Remove(Finish);
		GetGame().GetCallqueue().Remove(UpdateSound);

		if (m_bControlsDisabled)
			SetControlsDisabled(false);

		if (m_bMuteSound)
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 1);

		if (m_wScreen)
		{
			m_wScreen.RemoveFromHierarchy();
			m_wScreen = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A sequence interrupted by the end of the game must not leave the player blind and deaf.
	void ~GDC_TimeSkipSequence()
	{
		if (m_bRunning)
			CleanUp();
	}
}
